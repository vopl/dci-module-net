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
    class Links;
    class Routes;

    namespace enumerator
    {
        class Rtnetlink
        {
        public:
            Rtnetlink(Links* links, Routes* routes);
            ~Rtnetlink();

        private:
            void doNextRequest();
            bool request(uint32 type);
            void onSock(int fd, poll::descriptor::ReadyStateFlags readyState);


        private:
            Links *                             _links {};
            Routes *                            _routes {};
            std::unique_ptr<poll::Descriptor>   _sock {};
            sockaddr_nl                         _address {};
            iovec                               _msgiov {};
            msghdr                              _msg {};

        private:
            enum StateFlags
            {
                sf_null             = 0x00,

                sf_requestLink      = 0x01,
                sf_requestAddr      = 0x02,
                sf_requestRoute     = 0x04,

                sf_responseLink     = 0x10,
                sf_responseAddr     = 0x20,
                sf_responseRoute    = 0x40,
            };

            int _state {};
        };
    }
}
