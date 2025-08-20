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

#pragma once

#include "lua_table.hpp"

#include "nexuslua_export.h"

#include <string>
#include <string_view>

namespace nexuslua
{
    /// \brief This type is the actual message type that is sent by function \ref send
    /// \details If the message contains a [sub table](https://cbeam.org/doxygen/structcbeam_1_1container_1_1nested__map.html#ab0fa6f316c118fb58987dcd2d3ef12da) \ref nexuslua::LuaTable::replyToTableId "reply_to" with
    /// entries \ref nexuslua::LuaTable::agentNameId "agent" and \ref nexuslua::LuaTable::agentMessageId "message",
    /// the original message will be included in the result message in a SubTable named \ref nexuslua::Message::originalMessageTableId "original_message"
    /// with entries \ref nexuslua::Message::originalMessageNameId "message_name"
    /// and \ref nexuslua::Message::originalMessageParametersId "parameters".
    struct NEXUSLUA_EXPORT Message
    {
    public:
        Message(int agent_n, const std::string& my_name, const LuaTable& my_parameters)
            : agent_n{agent_n}
            , name{my_name}
            , parameters{my_parameters}
        {
        }

        Message(int agent_n, const Message& message)
            : agent_n{agent_n}
            , name{message.name}
            , parameters{message.parameters}
        {
        }

        Message(int agent_n)
            : agent_n{agent_n}
        {
        }

        Message()          = default;
        virtual ~Message() = default;

        int         agent_n = -1;
        std::string name;       ///< name of the message; second parameter of \ref send
        LuaTable    parameters; ///< parameter table of the message; third parameter of \ref send

        std::string GetOriginalMessageNameOrEmpty() const;       ///< return the original message name, in case this message is a reply to a message that specified a \ref nexuslua::LuaTable::replyToTableId "reply_to" sub table
        LuaTable    GetOriginalMessageParametersOrEmpty() const; ///< return the original message parameters, in case this message is a reply to a message that specified a \ref nexuslua::LuaTable::replyToTableId "reply_to" sub table

        virtual std::shared_ptr<Message> clone() const
        {
            return std::make_shared<Message>(agent_n, name, parameters);
        }

        static constexpr std::string_view originalMessageTableId{"original_message"}; ///< name of the SubTable that stores the original message, in case this message is a reply to a message that specified a \ref nexuslua::LuaTable::replyToTableId "reply_to" sub table
        static constexpr std::string_view originalMessageNameId{"message_name"};      ///< name of an entry that stores the original message name, in case this message is a reply to a message that specified a \ref nexuslua::LuaTable::replyToTableId "reply_to" sub table
        static constexpr std::string_view originalMessageParametersId{"parameters"};  ///< name of an entry that stores the original message parameters, in case this message is a reply to a message that specified a \ref nexuslua::LuaTable::replyToTableId "reply_to" sub table
    };
}
