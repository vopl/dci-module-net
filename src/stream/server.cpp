/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "server.hpp"
#include "../host.hpp"
#include "../utils/sockaddr.hpp"
#include "../utils/makeError.hpp"

namespace dci::module::net::stream
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Server::Server(Host* host)
        : api::stream::Server<>::Opposite(idl::interface::Initializer())
        , _host{host}
        , _sock{-1, [this](int fd, std::uint_fast32_t readyState) {sockReady(fd, readyState);}}
    {
        _host->track(this);

        methods()->setOption() += this * [&](const api::Option& op)
        {
            if(_sock.valid())
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

        methods()->listen() += this * [&](auto&& endpoint)
        {
            return listen(std::forward<decltype(endpoint)>(endpoint));
        };

        methods()->localEndpoint() += this * [&]()
        {
            return cmt::readyFuture(_localEndpoint);
        };

        methods()->close() += this * [&]()
        {
            close();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Server::~Server()
    {
        sbs::Owner::flush();
        _host->untrack(this);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    cmt::Future<void> Server::listen(auto&& endpoint)
    {
        close();

        _bindEndpoint = std::forward<decltype(endpoint)>(endpoint);

        union
        {
            sockaddr            _base;
            sockaddr_un         _un;
            sockaddr_storage    _space;
        } saddr;
        socklen_t saddrLen = utils::sockaddr::convert(_bindEndpoint, &saddr._base);

        int sock = ::socket(saddr._base.sa_family, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);

        if(-1 == sock)
        {
            return utils::fetchSystemError<void>();
        }

        std::error_code ec = _sock.attach(sock);
        if(ec)
        {
            return utils::makeError<void>(ec);
        }

        ExceptionPtr e = applyOptions(_sock);
        if(e)
        {
            return cmt::readyFuture<void>(e);
        }

        if(AF_LOCAL == saddr._base.sa_family)
        {
            unlink(saddr._un.sun_path);
        }

        if(::bind(sock, &saddr._base, saddrLen))
        {
            return utils::fetchSystemError<void>();
        }

        saddrLen = sizeof(saddr);
        if(::getsockname(sock, &saddr._base, &saddrLen))
        {
            return utils::fetchSystemError<void>();
        }
        utils::sockaddr::convert(&saddr._base, saddrLen, _localEndpoint);

        if(::listen(sock, 5))
        {
            return utils::fetchSystemError<void>();
        }

        return cmt::readyFuture<void>();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Server::close()
    {
        if(api::LocalEndpoint * l = std::get_if<api::LocalEndpoint>(&_bindEndpoint.std()))
        {
            unlink(l->address.data());
        }
        _sock.close();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Server::sockReady(int fd, std::uint_fast32_t readyState)
    {
        if(poll::Descriptor::rsf_error & readyState)
        {
            std::error_code ec = _sock.error();
            if(ec)
            {
                methods()->failed(utils::makeError(ec));
            }

            close();
            return;
        }

        if(poll::Descriptor::rsf_eof & readyState)
        {
            methods()->closed();
            _bindEndpoint = api::Endpoint();
            _localEndpoint = api::Endpoint();
            return;
        }

        if(poll::Descriptor::rsf_read & readyState)
        {
            for(;;)
            {
                union
                {
                    sockaddr            _base;
                    sockaddr_storage    _space;
                } saddr;
                ::socklen_t saddrLen = sizeof(saddr);

                int sock = ::accept4(fd, &saddr._base, &saddrLen, SOCK_NONBLOCK|SOCK_CLOEXEC);
                if(-1 != sock)
                {
                    api::Endpoint remoteEndpoint;
                    utils::sockaddr::convert(&saddr._base, saddrLen, remoteEndpoint);

                    stream::Channel* c = new stream::Channel(_host, sock, _localEndpoint, std::move(remoteEndpoint));
                    c->involvedChanged() += c * [c](bool v)
                    {
                        if(!v)
                        {
                            delete c;
                        }
                    };
                    methods()->accepted(api::stream::Channel<>(*c));
                }
                else
                {
                    if(EAGAIN != errno)
                    {
                        methods()->failed(utils::fetchSystemError());
                    }
                    break;
                }
            }
        }
    }
}
