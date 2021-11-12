/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include <dci/mm/heap/allocable.hpp>
#include <dci/sbs.hpp>
#include <dci/poll.hpp>
#include <dci/logger.hpp>
#include <dci/host/module/entry.hpp>
#include <dci/host/exception.hpp>
#include <dci/utils/atScopeExit.hpp>
#include "net.hpp"

#include <memory>
#include <cstring>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <unistd.h>

#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <net/if.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <sys/eventfd.h>

namespace dci::module::net
{
    using namespace dci;

    namespace api = dci::idl::net;
}

