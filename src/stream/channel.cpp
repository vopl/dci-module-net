/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "channel.hpp"
#include "../host.hpp"
#include "../utils/sockaddr.hpp"
#include "../utils/makeError.hpp"

namespace dci::module::net::stream
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::Channel(Host* host, int sock, const api::Endpoint& localEndpoint, api::Endpoint&& remoteEndpoint)
        : api::stream::Channel<>::Opposite(idl::interface::Initializer())
        , _host{host}
        , _sock{sock}
        , _localEndpoint{localEndpoint}
        , _remoteEndpoint{std::move(remoteEndpoint)}
        , _connectPromise{cmt::PromiseNullInitializer{}}
        , _connected{-1 != _sock}
    {
        if(_connected)
        {
            _sock.onAct() += _sockActOwner * [this](int fd, std::uint_fast32_t readyState){connectedSockReady(fd, readyState);};
        }

        _host->track(this);

        methods()->setOption() += this * [&](const api::Option& op)
        {
            if(_connected)
            {
                ExceptionPtr e = applyOption(_sock, op);
                if(e)
                {
                    return cmt::readyFuture<void>(e);
                }

                return cmt::readyFuture();
            }

            pushOption(op);
            return cmt::readyFuture();
        };

        methods()->localEndpoint() += this * [&]()
        {
            return cmt::readyFuture(_localEndpoint);
        };

        methods()->remoteEndpoint() += this * [&]()
        {
            return cmt::readyFuture(_remoteEndpoint);
        };

        methods()->send() += this * [&](auto&& bytes)
        {
            if(!_connected)
            {
                failed(utils::makeError<api::NotConnected>());
                return;
            }

            _sendBuffer.push(std::forward<decltype(bytes)>(bytes));
            if(poll::Descriptor::rsf_write & _lastReadyState)
            {
                doWrite(_sock.fd());
            }
        };

        methods()->startReceive() += this * [&]()
        {
            if(!_connected)
            {
                failed(utils::makeError<api::NotConnected>());
                return;
            }

            if(!_receiveStarted)
            {
                _receiveStarted = true;

                if(!_receivedData.empty())
                {
                    methods()->received(std::move(_receivedData));
                }
            }

            dbgAssert(_receivedData.empty());
        };

        methods()->stopReceive() += this * [&]()
        {
            _receiveStarted = false;
        };

        methods()->close() += this * [&]()
        {
            close();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    cmt::Future<api::stream::Channel<>> Channel::connect(bool needBind)
    {
        dbgAssert(!_connected);
        dbgAssert(!_sock.valid());
        dbgAssert(!_connectPromise.charged());

        int sock = ::socket(
                       utils::sockaddr::family(_remoteEndpoint),
                       SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,
                       0);

        if(-1 == sock)
        {
            return utils::fetchSystemError<api::stream::Channel<>>();
        }

        std::error_code ec = _sock.attach(sock);
        if(ec)
        {
            return utils::makeError<api::stream::Channel<>>(ec);
        }

        ExceptionPtr e = applyOptions(_sock);
        if(e)
        {
            return cmt::readyFuture<api::stream::Channel<>>(e);
        }

        if(needBind)
        {
            union
            {
                sockaddr            _base {};
                sockaddr_storage    _space;
            } saddr;

            socklen_t saddrLen = utils::sockaddr::convert(_localEndpoint, &saddr._base);
            if(::bind(sock, &saddr._base, saddrLen))
            {
                return utils::fetchSystemError<api::stream::Channel<>>();
            }
        }

        union
        {
            sockaddr            _base;
            sockaddr_storage    _space;
        } saddr;

        socklen_t saddrLen = utils::sockaddr::convert(_remoteEndpoint, &saddr._base);
        int res = ::connect(sock, &saddr._base, saddrLen);

        if(!res)
        {
            _connected = true;
            _sockActOwner.flush();
            _sock.onAct() += _sockActOwner * [this](int fd, std::uint_fast32_t readyState){connectedSockReady(fd, readyState);};
            return cmt::readyFuture(api::stream::Channel<>(*this));
        }

        if(EINPROGRESS != errno)
        {
            return utils::fetchSystemError<api::stream::Channel<>>();
        }

        _connectPromise = cmt::Promise<api::stream::Channel<>>();

        _connectPromise.canceled() += [this]
        {
            _connectPromise.uncharge();
            _sockActOwner.flush();
            _sock.close();
        };

        _sockActOwner.flush();
        _sock.onAct() += _sockActOwner * [this](int fd, std::uint_fast32_t readyState){connectSockReady(fd, readyState);};
        return _connectPromise.future();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::~Channel()
    {
        sbs::Owner::flush();
        _sockActOwner.flush();
        _sock.close();
        _host->untrack(this);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::failed(ExceptionPtr e, bool doClose)
    {
        api::stream::Channel<>::Opposite si = *this;
        bool emitClosed = false;

        if(doClose)
        {
            if(_connected)
            {
                emitClosed = true;
                _connected = false;
            }

            if(_connectPromise.charged() && !_connectPromise.resolved())
            {
                _connectPromise.resolveException(e);
            }

            if(!(poll::Descriptor::rsf_close & _lastReadyState))
            {
                _sockActOwner.flush();
                _sock.close();
            }

            _sendBuffer.clear();
        }

        si->failed(e);

        if(emitClosed)
        {
            si->closed();
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::close()
    {
        if(_connectPromise.charged() && !_connectPromise.resolved())
        {
            _connectPromise.resolveCancel();
        }

        if(!(poll::Descriptor::rsf_close & _lastReadyState))
        {
            _sockActOwner.flush();
            _sock.close();
        }

        _sendBuffer.clear();

        if(_connected)
        {
            _connected = false;
            methods()->closed();
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    bool Channel::doWrite(int fd)
    {
        dbgAssert(_sock.fd() == fd);
        dbgAssert(_lastReadyState & poll::Descriptor::rsf_write);

        if(_sendBuffer.empty())
        {
            return true;
        }

        uint32 totalWrote = 0;

        while(!_sendBuffer.empty())
        {
            msghdr msg = {nullptr, 0, _sendBuffer.iov(), _sendBuffer.iovAmount(), nullptr, 0, 0};
            ssize_t res = ::sendmsg(fd, &msg, MSG_NOSIGNAL);

            if(0 > res)
            {
                _lastReadyState &= ~poll::Descriptor::rsf_write;
                failed(utils::fetchSystemError(), true);
                return false;
            }

            uint32 wrote = static_cast<uint32>(res);
            totalWrote += wrote;

            if(0 == res)
            {
                break;
            }
            else if(static_cast<uint32>(res) < _sendBuffer.iovSize())
            {
                _lastReadyState &= ~poll::Descriptor::rsf_write;
                _sendBuffer.flush(wrote);
                break;
            }
            else
            {
                dbgAssert(wrote == _sendBuffer.iovSize());
                _sendBuffer.flush(wrote);
            }
        }

        if(totalWrote)
        {
            uint32 stillWait = _sendBuffer.totalSize();
            methods()->sended(totalWrote, stillWait);
        }

        return true;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    bool Channel::doRead(int fd)
    {
        dbgAssert(_sock.fd() == fd);
        dbgAssert(_lastReadyState & poll::Descriptor::rsf_read);

        _lastReadyState &= ~poll::Descriptor::rsf_read;

        uint32 totalReaded = 0;
        Bytes data;
        auto recvBuffer = _host->getRecvBuffer();

        for(;;)
        {
            ssize_t res = ::readv(fd, recvBuffer->iov(), static_cast<int>(recvBuffer->iovAmount()));

            if(0 > res)
            {
                if(EAGAIN == errno)
                {
                    //no more data
                    break;
                }

                failed(utils::fetchSystemError(), true);
                return false;
            }

            uint32 readed = static_cast<uint32>(res);
            totalReaded += readed;

            if(0 == readed)
            {
                break;
            }
            else if(readed < recvBuffer->totalSize())
            {
                recvBuffer->flushTo(data.end(), readed);
                break;
            }
            else
            {
                dbgAssert(readed == recvBuffer->totalSize());
                recvBuffer->flushTo(data.end());
            }
        }

        if(totalReaded)
        {
            if(_receiveStarted)
            {
                dbgAssert(_receivedData.empty());
                methods()->received(std::move(data));
            }
            else
            {
                _receivedData.end().write(std::move(data));
            }
        }
        else
        {
            //peer closed
            close();
            return false;
        }

        return true;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::connectSockReady(int /*fd*/, std::uint_fast32_t readyState)
    {
        dbgAssert(!_connected);

        dbgAssert(_connectPromise.charged());
        dbgAssert(!_connectPromise.resolved());

        auto connectPromise = std::exchange(_connectPromise, cmt::Promise<api::stream::Channel<>>(cmt::PromiseNullInitializer()));

        if(poll::Descriptor::rsf_error & readyState)
        {
            std::error_code ec = _sock.error();
            _sockActOwner.flush();
            _sock.close();

            connectPromise.resolveException(utils::makeError(ec));
            return;
        }

        if((poll::Descriptor::rsf_eof|poll::Descriptor::rsf_close) & readyState)
        {
            _sockActOwner.flush();
            _sock.close();

            connectPromise.resolveCancel();
            return;
        }

        if((poll::Descriptor::rsf_read|poll::Descriptor::rsf_write) & readyState)
        {
            _connected = true;
            _lastReadyState |= readyState;
            _sockActOwner.flush();
            _sock.onAct() += _sockActOwner * [this](int fd, std::uint_fast32_t readyState){connectedSockReady(fd, readyState);};

            connectPromise.resolveValue(api::stream::Channel<>(*this));
            return;
        }

        dbgWarn("never here");
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::connectedSockReady(int fd, std::uint_fast32_t readyState)
    {
        dbgAssert(_connected);

        _lastReadyState |= readyState;

        if(poll::Descriptor::rsf_error & _lastReadyState)
        {
            _lastReadyState = 0;

            std::error_code ec = _sock.error();
            if(ec)
            {
                failed(utils::makeError(ec), true);
            }
            else
            {
                close();
            }

            return;
        }

        if(poll::Descriptor::rsf_eof & _lastReadyState)
        {
            _lastReadyState = 0;

            close();
            return;
        }

        if(poll::Descriptor::rsf_write & _lastReadyState)
        {
            if(!doWrite(fd))
            {
                return;
            }
        }

        if(poll::Descriptor::rsf_read & _lastReadyState)
        {
            doRead(fd);
        }
    }
}
