/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "recvBuffer.hpp"

namespace dci::module::net::utils
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    RecvBuffer::RecvBuffer()
    {
        allocate(_iovAmount);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    RecvBuffer::~RecvBuffer()
    {
        for(bytes::Chunk* c : _chunks)
        {
            delete c;
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    iovec* RecvBuffer::iov()
    {
        return &_iov[0];
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 RecvBuffer::iovAmount()
    {
        return _iovAmount;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 RecvBuffer::totalSize()
    {
        return _totalSize;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void RecvBuffer::flushTo(bytes::Alter&& dst)
    {
        dst.write(_chunks[0], _chunks[_iovAmount-1], _totalSize);
        allocate(_iovAmount);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void RecvBuffer::flushTo(bytes::Alter&& dst, uint32 size)
    {
        dbgAssert(size > 0);
        dbgAssert(size < _totalSize);

        uint32 iovAmount = size / bytes::Chunk::bufferSize();
        if(iovAmount * bytes::Chunk::bufferSize() < size)
        {
            _chunks[iovAmount]->setEnd(static_cast<uint16>(size - iovAmount * bytes::Chunk::bufferSize()));
            iovAmount++;
        }
        _chunks[iovAmount-1]->setNext(nullptr);
        dst.write(_chunks[0], _chunks[iovAmount-1], size);

        allocate(iovAmount);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void RecvBuffer::allocate(uint32 iovAmount)
    {
        dbgAssert(iovAmount);

        bytes::Chunk* prev = nullptr;

        {
            bytes::Chunk *&cur = _chunks[0];
            cur = new bytes::Chunk(nullptr, nullptr, 0, bytes::Chunk::bufferSize());

            _iov[0].iov_base = cur->data();
            _iov[0].iov_len = bytes::Chunk::bufferSize();

            prev = cur;
        }

        for(uint32 i(1); i<iovAmount; ++i)
        {
            bytes::Chunk *&cur = _chunks[i];
            cur = new bytes::Chunk(nullptr, prev, 0, bytes::Chunk::bufferSize());

            _iov[i].iov_base = cur->data();
            _iov[i].iov_len = bytes::Chunk::bufferSize();

            prev->setNext(cur);
            prev = cur;
        }

        if(iovAmount < _iovAmount)
        {
            bytes::Chunk *&cur = _chunks[iovAmount];

            prev->setNext(cur);
            cur->setPrev(prev);
        }
    }

    RecvBuffer recvBuffer;
}
