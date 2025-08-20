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

#include "lua_table.hpp"

#include "message.hpp"

#include <cbeam/serialization/nested_map.hpp>
#include <cbeam/serialization/xpod.hpp>

namespace nexuslua
{
    using namespace std::string_literals;

    LuaTable::LuaTable(const cbeam::serialization::serialized_object serializedNestedMap)
    {
        cbeam::serialization::serialized_object it = serializedNestedMap;
        cbeam::serialization::traits<cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>>::deserialize(it, *this);
    }

    LuaTable::LuaTable(const cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>& baseInstance)
        : cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>(baseInstance)
    {
    }

    void LuaTable::SetOriginalMessage(const std::shared_ptr<Message> originalMessage)
    {
        if (originalMessage)
        {
            sub_tables[(std::string)Message::originalMessageTableId].data[(std::string)Message::originalMessageNameId]             = originalMessage->name;
            sub_tables[(std::string)Message::originalMessageTableId].sub_tables[(std::string)Message::originalMessageParametersId] = originalMessage->parameters;
        }
    }

    void LuaTable::SetReplyTo(const std::string& agentName, const std::string& messageName)
    {
        SetReplyToAgentName(agentName);
        SetReplyToMessageName(messageName);
    }

    void LuaTable::SetReplyToAgentName(const std::string& agentName)
    {
        sub_tables[(std::string)replyToTableId].data[(std::string)agentNameId] = agentName;
    }

    void LuaTable::SetReplyToMessageName(const std::string& messageName)
    {
        sub_tables[(std::string)replyToTableId].data[(std::string)agentMessageId] = messageName;
    }

    std::string LuaTable::GetReplyToAgentNameOrEmpty() const
    {
        const auto& itReplyToTable = sub_tables.find((std::string)replyToTableId);

        if (itReplyToTable == sub_tables.end())
        {
            return ""s;
        }

        return itReplyToTable->second.get_mapped_value_or_default<std::string>((std::string)agentNameId);
    }

    bool LuaTable::RequestsUnreplicatedReceiver() const
    {
        return get_mapped_value_or_default<bool>((std::string)unreplicatedId);
    }

    std::string LuaTable::GetReplyToMessageNameOrEmpty() const
    {
        const auto& itReplyToTable = sub_tables.find((std::string)replyToTableId);

        if (itReplyToTable == sub_tables.end())
        {
            return ""s;
        }

        return itReplyToTable->second.get_mapped_value_or_default<std::string>((std::string)agentMessageId);
    }

    LuaTableBase LuaTable::GetTableToMergeWhenReplyingOrEmpty() const
    {
        const auto& itReplyToTable = sub_tables.find((std::string)replyToTableId);

        if (itReplyToTable != sub_tables.end())
        {
            auto const& itMergeTable = itReplyToTable->second.sub_tables.find((std::string)tableToMergeWhenReplyingId);

            if (itMergeTable != itReplyToTable->second.sub_tables.end())
            {
                return itMergeTable->second;
            }
        }

        return {};
    }
}
