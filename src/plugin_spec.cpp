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

#include "plugin_spec.hpp"
#include "description.hpp"

#include <cbeam/filesystem/io.hpp>
#include <cbeam/filesystem/path.hpp>
#include <cbeam/platform/system_folders.hpp>

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

#include <stdexcept>

namespace nexuslua
{
    const char* PluginSpec::NameOfPersistentSubFolder = "persistent";

    // Constructor for local plugins (reads from disk)
    PluginSpec::PluginSpec(const std::filesystem::path& pluginPath)
    {
        const std::string     specFileName = "nexuslua_plugin.toml";
        std::filesystem::path specPath     = pluginPath / specFileName;

        if (!std::filesystem::exists(specPath))
        {
            throw std::runtime_error("Invalid plugin: " + pluginPath.string() + " does not contain " + specFileName);
        }

        const std::string toml_content = cbeam::filesystem::read_file(specPath);
        init(toml_content, false); // false = not an online source
    }

    PluginSpec::PluginSpec(const std::string& plugin_spec_toml_content, bool online_source)
    {
        init(plugin_spec_toml_content, online_source);
    }

    PluginSpec::PluginSpec(const std::string& plugin_spec_toml_content, std::string resolved_download_url)
    {
        init(plugin_spec_toml_content, true);
        url_download = std::move(resolved_download_url);
    }

    void PluginSpec::init(std::string plugin_spec_toml_content, bool online_source)
    {
        // Correctly handle the parse_result from toml::parse
        toml::parse_result result = toml::parse(plugin_spec_toml_content);
        if (!result)
        {
            throw std::runtime_error(std::string("Failed to parse plugin spec toml: ") + result.error().description().data());
        }

        // Move the successfully parsed table from the result
        toml::table tbl = std::move(result).table();

        display_name = tbl["displayName"].value_or<std::string>("");
        is_freeware  = tbl["isFreeware"].value_or<bool>(false);
        url_help     = tbl["urlHelp"].value_or<std::string>("");
        url_license  = tbl["urlLicense"].value_or<std::string>("");
        url_purchase = tbl["urlPurchase"].value_or<std::string>("");

        if (online_source)
        {
            version_online = tbl["version"].value_or<std::string>("");
        }
        else
        {
            version_installed = tbl["version"].value_or<std::string>("");
        }

        if (display_name.empty())
        {
            throw std::runtime_error("Plugin spec is missing mandatory field 'displayName'.");
        }

        _initialized = true;
    }

    void PluginSpec::ValidateInitialization() const
    {
        if (!_initialized)
        {
            throw std::logic_error("Internal error: using uninitialized PluginSpec");
        }
    }

    std::filesystem::path PluginSpec::GetInstallFolder() const
    {
        ValidateInitialization();
        return cbeam::filesystem::path(cbeam::filesystem::get_user_data_dir() / description::GetProductName() / "plugins" / display_name);
    }

    std::filesystem::path PluginSpec::GetPersistentFolder() const
    {
        ValidateInitialization();
        std::filesystem::path persistentFolder = std::filesystem::path(GetInstallFolder()) / NameOfPersistentSubFolder;
        cbeam::filesystem::path(persistentFolder).create_directory();
        return persistentFolder;
    }

    std::string PluginSpec::GetName() const
    {
        ValidateInitialization();
        return display_name;
    }
    std::string PluginSpec::GetVersionOnline() const
    {
        ValidateInitialization();
        return version_online;
    }
    std::string PluginSpec::GetVersionInstalled() const
    {
        ValidateInitialization();
        return version_installed;
    }
    void PluginSpec::SetVersionInstalled(const std::string& version)
    {
        version_installed = version;
    }
    bool PluginSpec::IsFreeware() const
    {
        ValidateInitialization();
        return is_freeware;
    }
    std::string PluginSpec::GetUrlHelp() const
    {
        ValidateInitialization();
        return url_help;
    }
    std::string PluginSpec::GetUrlDownload() const
    {
        ValidateInitialization();
        return url_download;
    }
    std::string PluginSpec::GetUrlLicense() const
    {
        ValidateInitialization();
        return url_license;
    }
    std::string PluginSpec::GetUrlPurchase() const
    {
        ValidateInitialization();
        return url_purchase;
    }
}