/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"

namespace dci::module::net
{
    class Host;

    class Routes
        : public sbs::Owner
    {
    public:
        Routes(api::Host<>::Opposite* iface);
        ~Routes();

        void add(const api::route::Entry4& e);
        void del(const api::route::Entry4& e);

        void add(const api::route::Entry6& e);
        void del(const api::route::Entry6& e);

        void flushChanges();

    private:
        api::Host<>::Opposite * _iface = nullptr;

        List<api::route::Entry4> _table4;
        List<api::route::Entry6> _table6;

        List<api::route::Entry4> _added4;
        List<api::route::Entry6> _added6;

        List<api::route::Entry4> _deleted4;
        List<api::route::Entry6> _deleted6;
    };
}
