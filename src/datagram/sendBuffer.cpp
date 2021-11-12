/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

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
        dbgAssert(!_iovAmount);
        _iovAmount = 0;

        for(;;)
        {
            if(src.atEnd())
            {
                if(_iovAmount)
                {
                    return SourceUtilization::full;
                }

                return SourceUtilization::none;
            }

            if(_iovAmount >= _iovAmountMax)
            {
                return SourceUtilization::partial;
            }

            _iov[_iovAmount].iov_base = const_cast<byte *>(src.continuousData());
            _iov[_iovAmount].iov_len = src.continuousDataSize();

            src.advanceChunks(1);
            _iovAmount++;
        }

        dbgWarn("unreacheable");
        return SourceUtilization::none;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    iovec* SendBuffer::iov()
    {
        return &_iov[0];
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 SendBuffer::iovAmount() const
    {
        return _iovAmount;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::clear()
    {
        _iovAmount = 0;
    }
}
