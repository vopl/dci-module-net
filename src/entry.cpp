/* This file is part of the the dci project. Copyright (C) 2013-2022 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "net.hpp"
#include "host.hpp"
#include "utils/makeError.hpp"

#include "net-stiac-support.hpp"

namespace dci::module::net
{
    namespace
    {
        struct Manifest
            : public dci::host::module::Manifest
        {
            Manifest()
            {
                _valid = true;
                _name = dciModuleName;
                _mainBinary = dciUnitTargetFile;

                pushServiceId<dci::idl::net::Host>();
            }
        } manifest_;


        struct Entry
            : public dci::host::module::Entry
        {
            Host * _host = nullptr;

            const Manifest& manifest() override
            {
                return manifest_;
            }

            bool load() override
            {
                return dci::host::module::Entry::load();
            }

            bool unload() override
            {
                if(_host)
                {
                    delete std::exchange(_host, nullptr);
                }

                return dci::host::module::Entry::unload();
            }

            bool stop() override
            {
                if(_host)
                {
                    delete std::exchange(_host, nullptr);
                }

                return dci::host::module::Entry::stop();
            }

            dci::cmt::Future<dci::idl::Interface> createService(dci::idl::ILid ilid) override
            {
                if(dci::idl::net::Host<>::lid() == ilid)
                {
                    if(!_host)
                    {
                        _host = new Host;
                    }

                    return dci::cmt::readyFuture(dci::idl::Interface(*_host));
                }

                return dci::host::module::Entry::createService(ilid);
            }
        } entry_;
    }
}

extern "C"
{
    DCI_INTEGRATION_APIDECL_EXPORT dci::host::module::Entry* dciModuleEntry = &dci::module::net::entry_;
}
