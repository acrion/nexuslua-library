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

#include <cbeam/container/xpod.hpp>

#include <cbeam/container/nested_map.hpp>

#include <cbeam/serialization/nested_map.hpp>

#include "nexuslua_export.h"

#include <memory>
#include <string>
#include <string_view>

namespace nexuslua
{
    struct Message;
    using LuaTableBase = cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>;

    /// \brief This type is used for the parameters of nexuslua::Message.
    /// In its serialized representation, it can be passed to or returned by functions of shared libraries that are imported via nexuslua function `import`.
    /// It is the composition of a std::map of cbeam::container::xpod::type instances (cbeam::container::table_of_values) and a std::map of cbeam::container::nested_map instances (cbeam::container::nested_table).
    /// The std::variant cbeam::container::xpod::type which is used for keys and values is central to the functionality of nexuslua.
    /// \details It serves the following purposes:
    /// \li carry data inside nexuslua::Message instances, i.e. the data in an cbeam::serialization::serialized_object
    /// \li interface between Lua code and C++ code (because these messages can be sent from Lua to Lua or between C++ and Lua)
    /// \li pass data from and to functions of shared libraries that have been imported using \ref import (by using data type cbeam::serialization::serialized_object, which is automatically serialized).
    struct NEXUSLUA_EXPORT LuaTable : public LuaTableBase
    {
        /// \brief construct empty LuaTable
        LuaTable() = default;

        /// \brief construct table from serialized void*
        LuaTable(const cbeam::serialization::serialized_object serializedNestedMap);

        /// \brief construct LuaTable from an instance of its base class
        LuaTable(const cbeam::container::nested_map<cbeam::container::xpod::type, cbeam::container::xpod::type>& baseInstance);

        void         SetOriginalMessage(const std::shared_ptr<Message> originalMessage);       ///< Copies the given message into a sub table (cbeam::container::nested_map::sub_tables) with name \ref nexuslua::Message::originalMessageTableId "\"original_message\"". Its name is copied to key \ref nexuslua::Message::originalMessageNameId "\"message_name\"" and its parameters into a cbeam::container::nested_map::sub_tables entry \ref nexuslua::Message::originalMessageParametersId "\"parameters\""
        void         SetReplyTo(const std::string& agentName, const std::string& messageName); ///< Sets the entries \ref agentNameId "\"reply_to/agent\"" and \ref agentMessageId "\"reply_to/message\"" to the given strings, which will trigger an automatic reply-to message with the return value of a function.
        void         SetReplyToAgentName(const std::string& agentName);                        ///< only sets the entry \ref agentNameId "\"reply_to/agent\"" to the given name, leaves the \ref agentMessageId "\"reply_to/message\"" unchanged
        void         SetReplyToMessageName(const std::string& messageName);                    ///< only sets the entry \ref agentMessageId "\"reply_to/message\"" to the given message name, leaves the \ref agentNameId "\"reply_to/agent\"" unchanged
        std::string  GetReplyToAgentNameOrEmpty() const;                                       ///< if there is an entry \ref agentNameId "\"reply_to/agent\"", returns it, otherwise returns the empty string
        std::string  GetReplyToMessageNameOrEmpty() const;                                     ///< if there is an entry \ref agentMessageId "\"reply_to/message\"", returns it, otherwise returns the empty string
        bool         RequestsUnreplicatedReceiver() const;                                     ///< return true if the table represents message parameters from a sender that requests that the message must be received by a non-replicated instance of the lua script that contains the message function
        LuaTableBase GetTableToMergeWhenReplyingOrEmpty() const;                               ///< if there is a table entry \ref tableToMergeWhenReplyingId "\"reply_to/merge\"", return it, otherwise an empty \ref nexuslua::LuaTableBase

    protected:
        static constexpr std::string_view replyToTableId{"reply_to"};          ///< name of a cbeam::container::nested_map::sub_tables entry that stores the agent that a message shall reply to
        static constexpr std::string_view tableToMergeWhenReplyingId{"merge"}; ///< name of a cbeam::container::nested_map::sub_tables entry that stores the agent that a message shall reply to
        static constexpr std::string_view unreplicatedId{"unreplicated"};      ///< name of a data field that stores if the sender requests that the message must be received by a non-replicated instance of the lua script that contains the message function
        static constexpr std::string_view agentNameId{"agent"};                ///< name of an entry in cbeam::serialization::serialized_object::data that stores the name of the agent that a message shall reply to
        static constexpr std::string_view agentMessageId{"message"};           ///< name of an entry in cbeam::serialization::serialized_object::data that stores the name of the message that shall be sent in reply to a message
    };
}