/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"
#include <regex>

namespace dci::module::net::ipResolver
{
    class Task
    {
    public:
        Task(auto&& src);
        ~Task();

        void doWorkInThread();

        template <class Value>
        auto init();

        void resolve();

    private:
        template <class Dst>
        bool fetchEndpoint(Dst& dst, addrinfo* src);

    public:
        Task*       _next {};

    private:
        String      _src;
        String      _host;
        String      _port;

        addrinfo    _hint {};
        addrinfo *  _res {};
        int         _resCode {};

        using PromiseVar = std::variant
        <
            char,
            cmt::Promise<api::IpEndpoint>,
            cmt::Promise<api::Ip4Endpoint>,
            cmt::Promise<api::Ip6Endpoint>,
            cmt::Promise<List<api::IpEndpoint>>,
            cmt::Promise<List<api::Ip4Endpoint>>,
            cmt::Promise<List<api::Ip6Endpoint>>
        >;
        PromiseVar _promiseVar;
        std::atomic<bool> _canceled = false;

    private:
        static const std::regex _regexHostService4;
        static const std::regex _regexHostService6;
    };



    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline Task::Task(auto&& src)
        : _src(std::forward<decltype(src)>(src))
    {
        /*
         * ip4
         * ip6
         * domain.name
         *
         * ip4:service
         * [ip6]:service
         * domain.name:service
         *
         */

        {
            std::smatch match;
            if(std::regex_match(_src, match, _regexHostService6))
            {
                _host = match[1];
                _port = match[2];

                return;
            }
        }

        {
            std::smatch match;
            if(std::regex_match(_src, match, _regexHostService4))
            {
                _host = match[1];
                _port = match[2];

                return;
            }
        }

        _host = _src;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Value>
    auto Task::init()
    {
        memset(&_hint, 0, sizeof(_hint));
        _hint.ai_flags = AI_ALL | AI_ADDRCONFIG;

        if constexpr(std::is_same_v<Value, api::Ip4Endpoint> || std::is_same_v<Value, List<api::Ip4Endpoint>>)
        {
            _hint.ai_family = AF_INET;
        }
        else if constexpr(std::is_same_v<Value, api::Ip6Endpoint> || std::is_same_v<Value, List<api::Ip6Endpoint>>)
        {
            _hint.ai_family = AF_INET6;
        }
        else
        {
            _hint.ai_family = AF_UNSPEC;
        }

        using Promise = cmt::Promise<Value>;
        Promise p;
        p.canceled() += [&]
        {
            _canceled.store(true, std::memory_order_release);
        };

        auto f = p.future();
        _promiseVar = std::move(p);
        return f;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <>
    inline bool Task::fetchEndpoint<api::Ip4Endpoint>(api::Ip4Endpoint& dst, addrinfo* src)
    {
        if(!src)
        {
            return false;
        }

        if(sizeof(sockaddr_in) == src->ai_addrlen && (PF_UNSPEC == src->ai_protocol || PF_INET == src->ai_protocol || (PF_PACKET == src->ai_protocol && AF_INET == src->ai_family)))
        {
            sockaddr_in* src4 = reinterpret_cast<sockaddr_in *>(src->ai_addr);
            dst.port = ntohs(src4->sin_port);
            memcpy(&dst.address.octets, &src4->sin_addr.s_addr, sizeof(dst.address.octets));
            return true;
        }

        return fetchEndpoint(dst, src->ai_next);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <>
    inline bool Task::fetchEndpoint<api::Ip6Endpoint>(api::Ip6Endpoint& dst, addrinfo* src)
    {
        if(!src)
        {
            return false;
        }

        if(sizeof(sockaddr_in6) == src->ai_addrlen && (PF_UNSPEC == src->ai_protocol || PF_INET6 == src->ai_protocol || (PF_PACKET == src->ai_protocol && AF_INET6 == src->ai_family)))
        {
            sockaddr_in6* src6 = reinterpret_cast<sockaddr_in6 *>(src->ai_addr);
            dst.port = ntohs(src6->sin6_port);
            memcpy(&dst.address.octets, &src6->sin6_addr.s6_addr, sizeof(dst.address.octets));
            dst.address.linkId = src6->sin6_scope_id;
            return true;
        }

        return fetchEndpoint(dst, src->ai_next);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <>
    inline bool Task::fetchEndpoint<api::IpEndpoint>(api::IpEndpoint& dst, addrinfo* src)
    {
        api::Ip4Endpoint dst4;
        if(fetchEndpoint(dst4, src))
        {
            dst = dst4;
            return true;
        }

        api::Ip6Endpoint dst6;
        if(fetchEndpoint(dst6, src))
        {
            dst = dst6;
            return true;
        }

        return false;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <>
    inline bool Task::fetchEndpoint<List<api::Ip4Endpoint>>(List<api::Ip4Endpoint>& dst, addrinfo* src)
    {
        if(!src)
        {
            return false;
        }

        bool res = false;
        if(sizeof(sockaddr_in) == src->ai_addrlen && (PF_UNSPEC == src->ai_protocol || PF_INET == src->ai_protocol || (PF_PACKET == src->ai_protocol && AF_INET == src->ai_family)))
        {
            sockaddr_in* src4 = reinterpret_cast<sockaddr_in *>(src->ai_addr);
            api::Ip4Endpoint dst4;
            dst4.port = ntohs(src4->sin_port);
            memcpy(&dst4.address.octets, &src4->sin_addr.s_addr, sizeof(dst4.address.octets));
            dst.push_back(dst4);
            res = true;
        }

        return fetchEndpoint(dst, src->ai_next) || res;
    }


    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <>
    inline bool Task::fetchEndpoint<List<api::Ip6Endpoint>>(List<api::Ip6Endpoint>& dst, addrinfo* src)
    {
        if(!src)
        {
            return false;
        }

        bool res = false;
        if(sizeof(sockaddr_in6) == src->ai_addrlen && (PF_UNSPEC == src->ai_protocol || PF_INET6 == src->ai_protocol || (PF_PACKET == src->ai_protocol && AF_INET6 == src->ai_family)))
        {
            sockaddr_in6* src6 = reinterpret_cast<sockaddr_in6 *>(src->ai_addr);
            api::Ip6Endpoint dst6;
            dst6.port = ntohs(src6->sin6_port);
            memcpy(&dst6.address.octets, &src6->sin6_addr.s6_addr, sizeof(dst6.address.octets));
            dst.push_back(dst6);
            res = true;
        }

        return fetchEndpoint(dst, src->ai_next) || res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <>
    inline bool Task::fetchEndpoint<List<api::IpEndpoint>>(List<api::IpEndpoint>& dst, addrinfo* src)
    {
        List<api::Ip4Endpoint> dst4;
        List<api::Ip6Endpoint> dst6;

        fetchEndpoint(dst4, src);
        fetchEndpoint(dst6, src);

        for(auto& a : dst4) dst.push_back(a);
        for(auto& a : dst6) dst.push_back(a);

        return true;
    }
}
