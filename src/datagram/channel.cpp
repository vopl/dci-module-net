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
#include "sendBuffer.hpp"

namespace dci::module::net::datagram
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::Channel(Host * host)
        : api::datagram::Channel<>::Opposite{idl::interface::Initializer{}}
        , _host{host}
        , _sock{-1, [this](int fd, std::uint_fast32_t readyState){sockReady(fd, readyState);}, nullptr}
    {
        _host->track(this);

        methods()->setOption() += this * [&](const api::Option& op)
        {
            if(_opened)
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

        methods()->bind() += this * [&](const api::Endpoint& ep)
        {
            _sock.close();

            ExceptionPtr e = open(&ep);
            if(e)
            {
                return cmt::readyFuture<void>(e);
            }

            return cmt::readyFuture<void>();
        };

        methods()->localEndpoint() += this * [&]()
        {
            if(!_opened)
            {
                return utils::makeError<api::Endpoint, api::Error>("not initialized");
            }

            if(!_localEndpointFetched)
            {
                union
                {
                    sockaddr            _base;
                    sockaddr_storage    _space;
                } saddr;
                socklen_t saddrLen = sizeof(saddr);
                if(::getsockname(_sock, &saddr._base, &saddrLen))
                {
                    return utils::fetchSystemError<api::Endpoint>();
                }
                utils::sockaddr::convert(&saddr._base, saddrLen, _localEndpoint);

                _localEndpointFetched = true;
            }

            return cmt::readyFuture(_localEndpoint);
        };

        methods()->send() += this * [&](const Bytes& data, const api::Endpoint& peer)
        {
            if(!_opened)
            {
                ExceptionPtr e = open(nullptr, &peer);
                if(e)
                {
                    failed(e);
                    return;
                }
            }

            doSend(_sock.fd(), std::forward<decltype(data)>(data), peer);
        };

        methods()->close() += this * [&]()
        {
            close();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::~Channel()
    {
        sbs::Owner::flush();
        _sock.close();
        _host->untrack(this);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::failed(ExceptionPtr e, bool doClose)
    {
        api::datagram::Channel<>::Opposite si = *this;
        bool emitClosed = false;

        if(doClose)
        {
            if(_opened)
            {
                emitClosed = true;
                _opened = false;
            }

            _sock.close();
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
        _sock.close();

        if(_opened)
        {
            _opened = false;
            methods()->closed();
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    ExceptionPtr Channel::open(const api::Endpoint* bind, const api::Endpoint* peer)
    {
        dbgAssert(!_sock.valid());
        dbgAssert(!_opened);
        dbgAssert(bind || peer);

        int sock = ::socket(utils::sockaddr::family(bind ? *bind : *peer), SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
        if(-1 == sock)
        {
            return utils::fetchSystemError();
        }

        std::error_code ec = _sock.attach(sock);
        if(ec)
        {
            _sock.close();
            return utils::makeError(ec);
        }

        ExceptionPtr e = applyOptions(_sock);
        if(e)
        {
            _sock.close();
            return e;
        }

        if(bind)
        {
            union
            {
                sockaddr            _base;
                sockaddr_storage    _space;
            } saddr;
            socklen_t saddrLen = utils::sockaddr::convert(*bind, &saddr._base);

            if(::bind(sock, &saddr._base, saddrLen))
            {
                std::exception_ptr e = utils::fetchSystemError();
                _sock.close();
                return e;
            }
        }

        _opened = true;
        return ExceptionPtr{};
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::doSend(int fd, const Bytes& data, const api::Endpoint& peer)
    {
        if(data.empty())
        {
            return;
        }

        union
        {
            sockaddr            _base;
            sockaddr_storage    _space;
        } saddr;
        socklen_t saddrLen = utils::sockaddr::convert(peer, &saddr._base);

        SendBuffer* sendBuffer = _host->getDatagramSendBuffer();
        msghdr msg = {&saddr._base, saddrLen, sendBuffer->iov(), 0, nullptr, 0, 0};

        bytes::Cursor src = data.begin();
        while(SendBuffer::SourceUtilization::partial == sendBuffer->fillFrom(src))
        {
            msg.msg_iovlen = sendBuffer->iovAmount();
            ssize_t res = ::sendmsg(fd, &msg, MSG_MORE);
            sendBuffer->clear();

            if(0 > res)
            {
                std::error_code ec = utils::fetchSystemErrorCode();
                failed(utils::makeError(ec), ec != std::errc::resource_unavailable_try_again);
                return;
            }
        }

        msg.msg_iovlen = sendBuffer->iovAmount();
        dbgAssert(msg.msg_iovlen);
        ssize_t res = ::sendmsg(fd, &msg, 0);
        sendBuffer->clear();

        if(0 > res)
        {
            std::error_code ec = utils::fetchSystemErrorCode();
            failed(utils::makeError(ec), ec != std::errc::resource_unavailable_try_again);
            return;
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::doRecv(int fd)
    {
        union
        {
            sockaddr            _base;
            sockaddr_storage    _space;
        } saddr;

        auto recvBuffer = _host->getRecvBuffer();
        msghdr msg = {&saddr._base, sizeof(saddr), recvBuffer->iov(), recvBuffer->iovAmount(), nullptr, 0, 0};

        ssize_t res = ::recvmsg(fd, &msg, 0);
        if(0 > res)
        {
            failed(utils::fetchSystemError(), true);
            return;
        }

        api::Endpoint peer;
        utils::sockaddr::convert(static_cast<sockaddr *>(msg.msg_name), msg.msg_namelen, peer);

        Bytes data;
        recvBuffer->flushTo(data.end(), static_cast<uint32>(res));

        methods()->received(std::move(data), peer);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Channel::sockReady(int fd, std::uint_fast32_t readyState)
    {
        if(poll::Descriptor::rsf_error & readyState)
        {
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

        if(poll::Descriptor::rsf_eof & readyState)
        {
            _sock.close();
            return;
        }

        if(poll::Descriptor::rsf_close & readyState)
        {
            _opened = false;
            _localEndpoint = api::Endpoint();
            _localEndpointFetched = false;
            _sock.close();
            methods()->closed();
            return;
        }

        if(poll::Descriptor::rsf_read & readyState)
        {
            doRecv(fd);
        }
    }
}
