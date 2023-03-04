/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"
#include "link.hpp"

namespace dci::module::net
{
    class Host;

    class Links
        : public sbs::Owner
    {
    public:
        Links(api::Host<>::Opposite* iface);
        ~Links();

    public:
        Link* getLink(uint32 id);
        Link* allocLink(uint32 id);
        void allocatedLinkInitialized(uint32 id, Link* link);

        void delLink(uint32 id);

        void flushChanges();

    private:
        using Interfaces        = Map<uint32, api::Link<>>;
        using Implementations   = Map<uint32, std::unique_ptr<Link>>;
        using Ids               = Set<uint32>;

    private:
        api::Host<>::Opposite * _iface = nullptr;

        //work
        Interfaces          _interfaces;
        Implementations     _implementations;

        //changes
        Implementations     _added;
        Ids                 _deleted;
    };
}
