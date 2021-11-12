/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "ipResolver.hpp"

namespace dci::module::net
{
    using namespace ipResolver;

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    IpResolver::IpResolver(api::Host<>::Opposite* iface)
        : _iface(iface)
        , _notifier4Owner{
              eventfd(1, EFD_NONBLOCK|EFD_CLOEXEC),
              [this](int fd, std::uint_fast32_t /*readyState*/){
                eventfd_t value;
                eventfd_read(fd, &value);
                wakeOwner();
              },
              nullptr}
    {
        _workers.emplace_back(std::thread(&IpResolver::workerProc, this));

        (*_iface)->resolveIp() += this * [&](auto&& endpoint)
        {
            return execute<api::IpEndpoint>(std::forward<decltype(endpoint)>(endpoint));
        };

        (*_iface)->resolveIp4() += this * [&](String endpoint)
        {
            return execute<api::Ip4Endpoint>(std::forward<decltype(endpoint)>(endpoint));
        };

        (*_iface)->resolveIp6() += this * [&](String endpoint)
        {
            return execute<api::Ip6Endpoint>(std::forward<decltype(endpoint)>(endpoint));
        };

        (*_iface)->resolveAllIp() += this * [&](String endpoint)
        {
            return execute<List<api::IpEndpoint>>(std::forward<decltype(endpoint)>(endpoint));
        };

        (*_iface)->resolveAllIp4() += this * [&](String endpoint)
        {
            return execute<List<api::Ip4Endpoint>>(std::forward<decltype(endpoint)>(endpoint));
        };

        (*_iface)->resolveAllIp6() += this * [&](String endpoint)
        {
            return execute<List<api::Ip6Endpoint>>(std::forward<decltype(endpoint)>(endpoint));
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    IpResolver::~IpResolver()
    {
        flush();

        //cancel all
        {
            std::unique_lock l(_mtx);
            _stopFlag = true;
        }
        _notifier4Worker.notify_all();

        for(std::thread& t : _workers)
        {
            t.join();
        }
        _workers.clear();

        wakeOwner();
        dbgAssert(!_tasks4Owner);

        _tasks4Owner = _tasks4Worker;
        _tasks4Worker = nullptr;

        wakeOwner();
        dbgAssert(!_tasks4Owner);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void IpResolver::ensureWorkersRan()
    {
        //not implemented, one thread always exists
    }

    void IpResolver::workerProc()
    {
        std::unique_lock l(_mtx);
        while(!_stopFlag)
        {
            if(_tasks4Worker)
            {
                ipResolver::Task* t = _tasks4Worker;
                _tasks4Worker = t->_next;

                l.unlock();
                t->doWorkInThread();
                l.lock();

                t->_next = _tasks4Owner;
                _tasks4Owner = t;
                eventfd_write(_notifier4Owner, 1);
            }
            else
            {
                _notifier4Worker.wait(l);
            }
        }
    }


    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void IpResolver::wakeOwnerCb(void* cbData, int fd, std::uint_fast32_t /*readyState*/)
    {
        eventfd_t value;
        eventfd_read(fd, &value);

        static_cast<IpResolver*>(cbData)->wakeOwner();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void IpResolver::wakeOwner()
    {
        Task* t;
        {
            std::unique_lock l(_mtx);

            t = _tasks4Owner;
            _tasks4Owner = nullptr;
        }

        while(t)
        {
            Task* next = t->_next;
            t->resolve();
            delete t;
            t = next;
        }
    }
}
