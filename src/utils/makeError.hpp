/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "../pch.hpp"

namespace dci::module::net::utils
{
    template <class Exception>
    std::exception_ptr makeError()
    {
        return std::make_exception_ptr(Exception{});
    }

    template <class Exception>
    std::exception_ptr makeError(std::string&& what)
    {
        return std::make_exception_ptr(Exception{std::move(what)});
    }

    template <class FutureT, class Exception>
    cmt::Future<FutureT> makeError(std::string&& what)
    {
        return cmt::readyFuture<FutureT>(makeError<Exception>(std::move(what)));
    }

    inline std::exception_ptr makeError(const std::error_code& ec)
    {
        switch(std::errc(ec.default_error_condition().value()))
        {
        case std::errc::address_in_use                  : return makeError<api::AddressInIse>();
        case std::errc::address_not_available           : return makeError<api::AddressNotAvailable>();
        case std::errc::already_connected               : return makeError<api::AlreadyConnected>();
        case std::errc::bad_address                     : return makeError<api::BadAddress>();
        case std::errc::connection_aborted              : return makeError<api::ConnectionAborted>();
        case std::errc::connection_refused              : return makeError<api::ConnectionRefused>();
        case std::errc::connection_reset                : return makeError<api::ConnectionReset>();
        case std::errc::host_unreachable                : return makeError<api::HostUnreachable>();
        case std::errc::invalid_argument                : return makeError<api::InvalidArgument>();
        case std::errc::network_down                    : return makeError<api::NetworkDown>();
        case std::errc::network_reset                   : return makeError<api::NetworkReset>();
        case std::errc::network_unreachable             : return makeError<api::NetworkUnreachable>();
        case std::errc::not_connected                   : return makeError<api::NotConnected>();
        case std::errc::operation_canceled              : return makeError<api::OperationCanceled>();
        case std::errc::operation_in_progress           : return makeError<api::OperationInProgress>();
        case std::errc::operation_not_permitted         : return makeError<api::OperationNotPermitted>();
        case std::errc::operation_not_supported         : return makeError<api::OperationNotSupported>();
        case std::errc::permission_denied               : return makeError<api::PermissionDenied>();
        case std::errc::resource_unavailable_try_again  : return makeError<api::UnavaliableTryAgain>();
        case std::errc::timed_out                       : return makeError<api::TimedOut>();
        default: break;
        }

        return makeError<api::Error>(ec.message());
    }

    template <class FutureT>
    cmt::Future<FutureT> makeError(const std::error_code& ec)
    {
        return cmt::readyFuture<FutureT>(makeError(ec));
    }

    inline std::error_code fetchSystemErrorCode()
    {
        return std::error_code{errno, std::generic_category()};
    }

    inline std::exception_ptr fetchSystemError()
    {
        return makeError(fetchSystemErrorCode());
    }

    template <class FutureT>
    cmt::Future<FutureT> fetchSystemError()
    {
        return cmt::readyFuture<FutureT>(fetchSystemError());
    }
}
