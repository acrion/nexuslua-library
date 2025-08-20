/*
Copyright (c) 2025 acrion innovations GmbH
Authors: Stefan Zipproth, s.zipproth@acrion.ch

This file is part of nexuslua, see https://github.com/acrion/nexuslua and https://nexuslua.org

nexuslua is offered under a commercial and under the AGPL license.
For commercial licensing, contact us at https://acrion.ch/sales. For AGPL licensing, see below.

AGPL licensing:

nexuslua is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

nexuslua is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with nexuslua. If not, see <https://www.gnu.org/licenses/>.
*/

#include <cbeam/container/find.hpp>

#include "message.hpp"

#include <cbeam/serialization/xpod.hpp>

namespace nexuslua
{
    std::string Message::GetOriginalMessageNameOrEmpty() const
    {
        const auto& itOriginalMessage = parameters.sub_tables.find((std::string)originalMessageTableId);

        if (itOriginalMessage == parameters.sub_tables.end())
        {
            return "";
        }

        return itOriginalMessage->second.get_mapped_value_or_default<std::string>((std::string)originalMessageNameId);
    }

    LuaTable Message::GetOriginalMessageParametersOrEmpty() const
    {
        const auto& itOriginalMessage = parameters.sub_tables.find((std::string)originalMessageTableId);

        if (itOriginalMessage == parameters.sub_tables.end())
        {
            return {};
        }

        const auto& itOriginalMessageParameters = itOriginalMessage->second.sub_tables.find((std::string)originalMessageParametersId);

        if (itOriginalMessageParameters == itOriginalMessage->second.sub_tables.end())
        {
            return {};
        }

        return itOriginalMessageParameters->second;
    }
}
