/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "sendBuffer.hpp"

namespace dci::module::net::datagram
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    SendBuffer::SendBuffer()
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    SendBuffer::~SendBuffer()
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    SendBuffer::SourceUtilization SendBuffer::fillFrom(bytes::Cursor& src)
    {
        dbgAssert(!_bufsAmount);
        _bufsAmount = 0;

        for(;;)
        {
            if(src.atEnd())
            {
                if(_bufsAmount)
                {
                    return SourceUtilization::full;
                }

                return SourceUtilization::none;
            }

            if(_bufsAmount >= _bufsAmountMax)
            {
                return SourceUtilization::partial;
            }

            _bufs[_bufsAmount].data() = reinterpret_cast<Buf::Data>(const_cast<byte*>(src.continuousData()));
            _bufs[_bufsAmount].len() = src.continuousDataSize();

            src.advanceChunks(1);
            _bufsAmount++;
        }

        dbgWarn("unreacheable");
        return SourceUtilization::none;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Buf* SendBuffer::bufs()
    {
        return &_bufs[0];
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 SendBuffer::bufsAmount() const
    {
        return _bufsAmount;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::clear()
    {
        _bufsAmount = 0;
    }
}
