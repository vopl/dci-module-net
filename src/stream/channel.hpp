/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"
#include "../optionsStore.hpp"
#include "../utils/recvBuffer.hpp"
#include "sendBuffer.hpp"

namespace dci::module::net
{
    class Host;

    namespace stream
    {
        class Channel
            : public api::stream::Channel<>::Opposite
            , public sbs::Owner
            , public mm::heap::Allocable<Channel>
            , public OptionsStore
        {
        public:
            Channel(
                    Host* host,
                    int sock,
                    const api::Endpoint& localEndpoint,
                    api::Endpoint&& remoteEndpoint);

            ~Channel();

        public:
            cmt::Future<api::stream::Channel<>> connect(bool needBind);

        private:
            void failed(ExceptionPtr e, bool doClose = false);
            void close();

            bool doWrite(int fd);
            bool doRead(int fd);

            //static void s_connectSockReady(void* self, int fd, std::uint_fast32_t readyState);
            void connectSockReady(int fd, std::uint_fast32_t readyState);

            //static void s_connectedSockReady(void* self, int fd, std::uint_fast32_t readyState);
            void connectedSockReady(int fd, std::uint_fast32_t readyState);

        private:
            Host *              _host;
            poll::Descriptor    _sock;
            sbs::Owner          _sockActOwner;
            api::Endpoint       _localEndpoint;
            api::Endpoint       _remoteEndpoint;

            SendBuffer          _sendBuffer;

            std::uint_fast32_t  _lastReadyState = 0;

            cmt::Promise<api::stream::Channel<>>
                                _connectPromise;

            bool                _connected = false;

            bool                _receiveStarted = false;
            Bytes               _receivedData;
        };
    }
}
