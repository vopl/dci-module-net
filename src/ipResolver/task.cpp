/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "task.hpp"
#include "../utils/makeError.hpp"

namespace dci::module::net::ipResolver
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    const std::regex Task::_regexHostService4{"^([^\\:]+)(?:\\:(.*))?$", std::regex::optimize};

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    const std::regex Task::_regexHostService6{"^\\[([^\\]]*)\\](?:\\:(.*))?$", std::regex::optimize};

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Task::~Task()
    {
        if(_res)
        {
            freeaddrinfo(_res);
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Task::doWorkInThread()
    {
        if(_canceled.load(std::memory_order_consume))
        {
            return;
        }

        dbgAssert(!_res);
        _resCode = getaddrinfo(
                    _host.data(),
                    _port.data(),
                    &_hint,
                    &_res);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Task::resolve()
    {
        if(_canceled.load(std::memory_order_consume))
        {
            return;
        }

        std::visit([&](auto& p)
        {
            if constexpr(std::is_same_v<char&, decltype(p)>)
            {
                return;
            }
            else
            {
                if(p.resolved())
                {
                    return;
                }

                if(_resCode)
                {
                    p.resolveException(utils::makeError<api::ResolveError>(gai_strerror(_resCode)));
                    return;
                }

                typename std::remove_reference_t<decltype(p)>::Value v {};
                fetchEndpoint(v, _res);
                p.resolveValue(std::move(v));
            }

        }, _promiseVar);
    }
}
