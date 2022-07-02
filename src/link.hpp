/* This file is part of the the dci project. Copyright (C) 2013-2022 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"

namespace dci::module::net
{
    class Link
        : public api::Link<>::Opposite
        , public sbs::Owner
        , public mm::heap::Allocable<Link>
    {
    public:
        Link(uint32 id);
        ~Link();

        void setHwAddress(const List<uint8>& v);
        void setName(const std::string& v);
        void setMtu(uint32 v);
        void setFlags(api::link::Flags v);

        void setIp4(List<api::link::Ip4Address> addresses);
        void addIp4(api::link::Ip4Address address);
        void delIp4(api::link::Ip4Address address);

        void setIp6(List<api::link::Ip6Address> addresses);
        void addIp6(api::link::Ip6Address address);
        void delIp6(api::link::Ip6Address address);

        void flushChanges();
        void remove();

    private:
        uint32                      _id = 0;
        List<uint8>                 _hwAddress;
        String                      _name;
        uint32                      _mtu = 0;
        api::link::Flags            _flags {};
        List<api::link::Ip4Address> _ip4;
        List<api::link::Ip6Address> _ip6;

        bool                        _changed = false;
    };
}
