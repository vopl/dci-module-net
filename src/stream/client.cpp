/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "client.hpp"
#include "../host.hpp"

namespace dci::module::net::stream
{
    Client::Client(Host* host)
        : api::stream::Client<>::Opposite{idl::interface::Initializer{}}
        , _host(host)
        , _binded(false)
    {
        _host->track(this);

        methods()->setOption() += this * [&](const api::Option& op)
        {
            pushOption(op);
            return cmt::readyFuture();
        };

        methods()->bind() += this * [this](auto&& endpoint)
        {
            _bindEndpoint = std::forward<decltype(endpoint)>(endpoint);
            _binded = true;
            return cmt::readyFuture<void>();
        };

        methods()->connect() += this * [this](auto&& endpoint)
        {
            stream::Channel* c = new stream::Channel{_host, {}, _bindEndpoint, api::Endpoint(std::forward<decltype(endpoint)>(endpoint))};
            c->involvedChanged() += c * [c](bool v)
            {
                if(!v)
                {
                    delete c;
                }
            };

            c->pushOptions(options());
            cmt::Future<api::stream::Channel<>> res = c->connect(_binded);
            res.then() += c * [c](cmt::Future<api::stream::Channel<>> in)
            {
                if(!in.resolvedValue())
                {
                    delete c;
                }
            };

            return res;
        };
    }

    Client::~Client()
    {
        sbs::Owner::flush();
        _host->untrack(this);
    }
}
