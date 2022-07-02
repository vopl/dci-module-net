/* This file is part of the the dci project. Copyright (C) 2013-2022 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"

namespace dci::module::net::stream
{
    class SendBuffer
    {
        SendBuffer(const SendBuffer&) = delete;
        void operator=(const SendBuffer&) = delete;

    public:
        SendBuffer();
        ~SendBuffer();

        void push(const Bytes& data);
        void push(Bytes&& data);

        void clear();
        bool empty() const;

        Buf* bufs();
        uint32 bufsAmount() const;
        uint32 bufsSize() const;

        uint32 totalSize() const;

        void flush(uint32 size);

    private:
        void enfillBufs();

    private:
        static constexpr uint32 _bufsAmountMin4Enfill = 16;
        static constexpr uint32 _bufsAmountMax = Buf::_maxBufs;

    private:
        Bytes   _data;

        Buf     _bufs[_bufsAmountMax];
        uint32  _bufsAmount = 0;
        uint32  _bufsSize = 0;
    };
}
