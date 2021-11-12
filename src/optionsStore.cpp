/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "optionsStore.hpp"
#include "utils/makeError.hpp"
#include "utils/sockaddr.hpp"

namespace dci::module::net
{

    namespace
    {
        template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
        template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    ExceptionPtr OptionsStore::applyOption(int fd, const api::Option& op)
    {
        return op.visit(overloaded
        {
            [&](const api::option::Broadcast& op)
            {
                int v = op.enable ? 1 : 0;
                if(::setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::ReuseAddr& op)
            {
                int v = op.enable ? 1 : 0;
                if(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::NoDelay& op)
            {
                int v = op.enable ? 1 : 0;
                if(::setsockopt(fd, SOL_TCP, TCP_NODELAY, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::Keepalive& op)
            {
                int v = op.enable ? 1 : 0;
                if(::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::KeepaliveIdle& op)
            {
                int v = op.seconds;
                if(::setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::KeepaliveInterval& op)
            {
                int v = op.seconds;
                if(::setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::KeepaliveCount& op)
            {
                int v = op.value;
                if(::setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::ReceiveBuf& op)
            {
                int v = static_cast<int>(op.size);
                if(::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::SendBuf& op)
            {
                int v = static_cast<int>(op.size);
                if(::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::Linger& op)
            {
                linger v { op.enable?1:0, op.timeoutSeconds };
                if(::setsockopt(fd, SOL_SOCKET, SO_LINGER, &v, sizeof(v))) return utils::fetchSystemError();
                return ExceptionPtr();
            },
            [&](const api::option::JoinMulticast& op)
            {
                if(op.group.holds<api::Ip4Address>())
                {
                    const api::Ip4Address& addr = op.group.get<api::Ip4Address>();
                    ip_mreq v {};
                    memcpy(&v.imr_multiaddr, addr.octets.data(), addr.octets.size());
                    if(::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &v, sizeof(v))) return utils::fetchSystemError();
                }
                else
                {
                    const api::Ip6Address& addr = op.group.get<api::Ip6Address>();
                    ipv6_mreq v {};
                    memcpy(&v.ipv6mr_multiaddr, addr.octets.data(), addr.octets.size());
                    v.ipv6mr_interface = addr.linkId;
                    if(::setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &v, sizeof(v))) return utils::fetchSystemError();
                }
                return ExceptionPtr();
            },
            [&](const api::option::LeaveMulticast& op)
            {
                if(op.group.holds<api::Ip4Address>())
                {
                    const api::Ip4Address& addr = op.group.get<api::Ip4Address>();
                    ip_mreq v {};
                    memcpy(&v.imr_multiaddr, addr.octets.data(), addr.octets.size());
                    if(::setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &v, sizeof(v))) return utils::fetchSystemError();
                }
                else
                {
                    const api::Ip6Address& addr = op.group.get<api::Ip6Address>();
                    ipv6_mreq v {};
                    memcpy(&v.ipv6mr_multiaddr, addr.octets.data(), addr.octets.size());
                    v.ipv6mr_interface = addr.linkId;
                    if(::setsockopt(fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &v, sizeof(v))) return utils::fetchSystemError();
                }
                return ExceptionPtr();
            },
            [&](const auto& op)
            {
                (void)op;
                return std::make_exception_ptr(api::InvalidArgument{"bad option provided"});
            }
        });
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    const OptionsStore::Options& OptionsStore::options() const
    {
        return _options;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void OptionsStore::pushOptions(const Options& ops)
    {
        _options.insert(_options.end(), ops.begin(), ops.end());
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void OptionsStore::pushOption(const api::Option& op)
    {
        _options.emplace_back(op);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    ExceptionPtr OptionsStore::applyOptions(int fd, bool flush)
    {
        ExceptionPtr res;

        for(const api::Option& op : _options)
        {
            res = applyOption(fd, op);
            if(res)
            {
                return res;
            }
        }

        if(flush)
        {
            std::vector<api::Option>().swap(_options);
        }

        return res;
    }
}
