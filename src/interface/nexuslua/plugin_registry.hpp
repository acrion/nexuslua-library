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

#include "plugin_install_result.hpp"

#include "nexuslua_export.h"

#include <cstddef>
#include <memory>
#include <string>

namespace nexuslua
{
    class Agent;
    class agents;

    /// \brief This class provides the interface to nexuslua "plugins" (agents that are installable from an online repository)
    class NEXUSLUA_EXPORT PluginRegistry
    {
        struct Impl;
        std::unique_ptr<Impl> _impl;

    public:
        /// \brief enable iteration over plugins using an instance of PluginsOnline
        struct NEXUSLUA_EXPORT Iterator
        {
            using iterator_category = std::input_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = std::shared_ptr<Agent>;
            /// \brief Type of the iterator's pointer-like object
            using pointer = struct
            {
                Impl*       impl;
                std::size_t pos;
            };

            /// \brief Construct a new Iterator object.
            /// \param ptr A pointer-like object of type `pointer` representing the iterator's position.
            explicit Iterator(pointer ptr)
                : m_ptr(ptr) {}

            /// \brief dereference operator
            /// \return the value pointed to by the iterator
            value_type operator*() const;

            /// \brief arrow operator
            /// \return the iterator’s pointer-like object
            pointer operator->() { return m_ptr; }

            /// prefix increment
            Iterator& operator++()
            {
                m_ptr.pos++;
                return *this;
            }

            /// postfix increment
            const Iterator operator++(int) // NOLINT(readability-const-return-type)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            /// \brief equality comparison operator
            friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_ptr.impl == b.m_ptr.impl && a.m_ptr.pos == b.m_ptr.pos; }

            /// \brief inequality comparison operator
            friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_ptr.impl != b.m_ptr.impl || a.m_ptr.pos != b.m_ptr.pos; }

        private:
            pointer m_ptr;
        };

        explicit PluginRegistry(const std::shared_ptr<nexuslua::agents>& agents);
        ~PluginRegistry();

        PluginInstallResult    Install(std::string name, std::string& strError); ///< install the given online plugin; store any errors in strError
        std::string            GetErrorMessage();                                ///< return the last error message
        std::shared_ptr<Agent> Get(const std::string& agentName);                ///< return the nexuslua::Agent with the given name, provided it is installed as a plugin
        std::size_t            Count() const;                                    ///< return the current number of plugins; depending if RescanInstalled() has been called, this may include plugins that are only available locally
        void                   RescanInstalled();                                ///< merge information about the installed plugins with the online plugins; this covers situations where a plugin has been installed that is not available online

        Iterator begin(); ///< required to iterator over plugins using a for loop
        Iterator end();   ///< required to iterator over plugins using a for loop
    };
}