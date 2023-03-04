/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include "pch.hpp"

namespace dci::module::net
{
    class OptionsStore
    {
        using Options = std::vector<api::Option>;

    public:
        static ExceptionPtr applyOption(poll::descriptor::Native native, const api::Option& op);

        const Options& options() const;
        void pushOptions(const Options& ops);
        void pushOption(const api::Option& op);
        ExceptionPtr applyOptions(poll::descriptor::Native native, bool flush=true);

    private:
        Options _options;
    };
}
