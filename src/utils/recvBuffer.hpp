/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"

namespace dci::module::net::utils
{
    class RecvBuffer
    {
        RecvBuffer(const RecvBuffer&) = delete;
        void operator=(const RecvBuffer&) = delete;

    public:
        RecvBuffer();
        ~RecvBuffer();

        Buf* bufs();
        uint32 bufsAmount();

        uint32 totalSize();

        void flushTo(bytes::Alter&& dst);
        void flushTo(bytes::Alter&& dst, uint32 size);

    private:
        void allocate(uint32 bufsAmount);

    private:
        static constexpr uint32 _bufsAmount = Buf::_maxBufs;
        static constexpr uint32 _totalSize = _bufsAmount * bytes::Chunk::bufferSize();

    private:
        bytes::Chunk*   _chunks[_bufsAmount];
        Buf             _bufs[_bufsAmount];
    };
}
