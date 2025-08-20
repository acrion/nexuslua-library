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

#include <filesystem>

namespace nexuslua
{
    /// \brief result values of agents::InstallPlugin
    enum class PluginInstallResult
    {
        SUCCESS,
        ERROR_PLUGIN_ALREADY_INSTALLED, ///< the target directory already exists. If user wants to update, he first has to uninstall that plugin
        ERROR_DIRECTORY_DOES_NOT_EXIST, ///< the source directory the user specified does not exist
        ERROR_COPYING_PLUGIN_TO_TARGET_DIRECTORY,
        ERROR_INVALID_SRC,
        ERROR_WHILE_CREATING_INSTANCE ///< in case of this result, parameter errorMessage of method InstallPlugin contains an English error message that has to be shown to the user (who is a plugin developer in this case)
    };

    /// \brief result values of agents::UninstallPlugin
    struct PluginUninstallResult
    {
        /// describes the possible results of a plugin uninstallation
        enum class Result
        {
            SUCCESS,
            ERROR_PLUGIN_IN_USE,                  ///< this application (or another process) currently uses the plugin, so it cannot be deleted. Tell user to quit and manually delete plugin directory AgentPlugin::GetPath()
            ERROR_INTERNAL_PLUGIN_DOES_NOT_EXIST, ///< the plugin name is not one of the names returned by AgentPlugin::GetName()
            ERROR_WHILE_UPDATING_PLUGINS_AFTER_UNINSTALL,
            ERROR_NO_ACTION_TAKEN
        };

        PluginUninstallResult()
            : result(Result::ERROR_NO_ACTION_TAKEN)
        {
        }

        /// constructs from the given result, in addition a path is provided the points to a backup of the uninstalled plugin. The backup can later be used to restore the `persistent` subfolder that may contain e. g. license files
        PluginUninstallResult(Result result, const std::filesystem::path& backup)
            : result(result)
            , backup(backup)
        {
        }

        Result                result; ///< contains the uninstallation result
        std::filesystem::path backup; ///< may contain a path to a backup of the uninstalled plugin. This can be later used to restore the `persistent` subfolder containing e. g. license files.
    };
} // namespace nexuslua
