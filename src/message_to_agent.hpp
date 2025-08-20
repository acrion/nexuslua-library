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

#include "message.hpp"

#include "nexuslua_export.h"

namespace nexuslua
{
    template <int N>
    class MessageToAgent : public Message
    {
    public:
        MessageToAgent(const std::string& name, const LuaTable& parameters)
            : Message(N, name, parameters)
        {
        }

        MessageToAgent()
            : Message(N)
        {
        }

        MessageToAgent(const MessageToAgent& messageToAgent)
            : Message(N, messageToAgent)
        {
        }

        MessageToAgent(const Message& message)
            : Message(message)
        {
            assert(N == message.agent_n && "nexuslua::message_to_agent: invalid conversion from Message base class");
        }

        std::shared_ptr<Message> clone() const override
        {
            return std::make_shared<MessageToAgent<N>>(name, parameters);
        }

        MessageToAgent<N>& operator=(const Message& other)
        {
            if (this != &other)
            {
                name       = other.name;
                parameters = other.parameters;
            }
            return *this;
        }

        static constexpr int AgentN = N;
    };
}
