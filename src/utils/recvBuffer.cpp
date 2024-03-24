/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "recvBuffer.hpp"

namespace dci::module::net::utils
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    RecvBuffer::RecvBuffer()
    {
        allocate(_bufsAmount);
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
    Buf* RecvBuffer::bufs()
    {
        return &_bufs[0];
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 RecvBuffer::bufsAmount()
    {
        return _bufsAmount;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    uint32 RecvBuffer::totalSize()
    {
        return _totalSize;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Bytes RecvBuffer::detach(uint32 size)
    {
        dbgAssert(size > 0);

        if(size >= _totalSize)
        {
            dci::utils::AtScopeExit after{[&]()
            {
                allocate(_bufsAmount);
            }};

            return Bytes{_chunks[0], _chunks[_bufsAmount-1], _totalSize};
        }

        uint32 bufsAmount = size / bytes::Chunk::bufferSize();
        if(bufsAmount * bytes::Chunk::bufferSize() < size)
        {
            _chunks[bufsAmount]->setEnd(static_cast<uint16>(size - bufsAmount * bytes::Chunk::bufferSize()));
            bufsAmount++;
        }
        _chunks[bufsAmount-1]->setNext(nullptr);

        dci::utils::AtScopeExit after{[&]()
        {
            allocate(bufsAmount);
        }};

        return Bytes{_chunks[0], _chunks[bufsAmount-1], size};
    }


    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void RecvBuffer::allocate(uint32 bufsAmount)
    {
        dbgAssert(bufsAmount);

        bytes::Chunk* prev = nullptr;

        {
            bytes::Chunk *&cur = _chunks[0];
            cur = new bytes::Chunk{nullptr, nullptr, 0, bytes::Chunk::bufferSize()};

            _bufs[0].data() = reinterpret_cast<Buf::Data>(cur->data());
            _bufs[0].len() = bytes::Chunk::bufferSize();

            prev = cur;
        }

        for(uint32 i(1); i<bufsAmount; ++i)
        {
            bytes::Chunk *&cur = _chunks[i];
            cur = new bytes::Chunk{nullptr, prev, 0, bytes::Chunk::bufferSize()};

            _bufs[i].data() = reinterpret_cast<Buf::Data>(cur->data());
            _bufs[i].len() = bytes::Chunk::bufferSize();

            prev->setNext(cur);
            prev = cur;
        }

        if(bufsAmount < _bufsAmount)
        {
            bytes::Chunk *&cur = _chunks[bufsAmount];

            prev->setNext(cur);
            cur->setPrev(prev);
        }
    }

    RecvBuffer recvBuffer;
}
