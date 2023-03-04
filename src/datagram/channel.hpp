/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"
#include "dci/poll/descriptor/native.hpp"
#include "../optionsStore.hpp"
#include "../utils/recvBuffer.hpp"

namespace dci::module::net
{
    class Host;

    namespace datagram
    {
        class Channel
            : public api::datagram::Channel<>::Opposite
            , public sbs::Owner
            , public mm::heap::Allocable<Channel>
            , private OptionsStore
        {
        public:
            Channel(Host* host);
            ~Channel();

        private:
            void failed(ExceptionPtr e, bool doClose = false);
            void close();

            ExceptionPtr open(const api::Endpoint* bind = nullptr, const api::Endpoint* peer = nullptr);
            void doSend(poll::descriptor::Native native, const Bytes& data, const api::Endpoint& peer);
            void doRecv(poll::descriptor::Native native);
            void sockReady(poll::descriptor::Native native, poll::descriptor::ReadyStateFlags readyState);

        private:
            Host *              _host;
            poll::Descriptor    _sock;
            api::Endpoint       _localEndpoint;

            bool                _opened = false;
            bool                _localEndpointFetched = false;
        };
    }
}
