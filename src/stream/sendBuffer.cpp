/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "sendBuffer.hpp"

namespace dci::module::net::stream
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    SendBuffer::SendBuffer()
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    SendBuffer::~SendBuffer()
    {
        clear();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::push(const Bytes& data)
    {
        return push(Bytes(data));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::push(Bytes&& data)
    {
        _data.end().write(std::move(data));

        if(_iovAmountMin4Enfill >= _iovAmount)
        {
            enfillIov();
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::clear()
    {
        _data.clear();
        _iovAmount = 0;
        _iovSize = 0;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    bool SendBuffer::empty() const
    {
        return !_iovSize;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    iovec* SendBuffer::iov()
    {
        dbgAssert(!empty());
        return &_iov[0];
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 SendBuffer::iovAmount() const
    {
        return _iovAmount;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 SendBuffer::iovSize() const
    {
        return _iovSize;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 SendBuffer::totalSize() const
    {
        return _data.size();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::flush(uint32 size)
    {
        dbgAssert(_iovSize >= size);
        dbgAssert(!empty());

        _data.begin().remove(size);
        enfillIov();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void SendBuffer::enfillIov()
    {
        _iovAmount = 0;
        _iovSize = 0;
        bytes::Cursor c(_data.begin());

        while(!c.atEnd() && _iovAmount < _iovAmountMax)
        {
            _iov[_iovAmount].iov_base = const_cast<byte *>(c.continuousData());
            _iov[_iovAmount].iov_len = c.continuousDataSize();

            _iovSize += _iov[_iovAmount].iov_len;
            _iovAmount++;
            c.advanceChunks(1);
        }
    }
}
