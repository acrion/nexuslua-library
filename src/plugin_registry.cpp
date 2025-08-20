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

#include "plugin_registry.hpp"

#include "agent_plugin.hpp"
#include "agents.hpp"
#include "description.hpp"
#include "plugin_spec.hpp"
#include "utility.hpp"

#include <cbeam/filesystem/io.hpp>
#include <cbeam/logging/log_manager.hpp>
#include <cbeam/platform/system_folders.hpp>

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

#include <set>
#include <sstream>

namespace nexuslua
{
    // The central URL to the plugin index file.
    const std::string REGISTRY_URL = "https://raw.githubusercontent.com/acrion/nexuslua-plugins/main/plugins.toml";

// Determine the correct TOML key for the download URL at compile-time.
#if defined(__linux__)
    const char* URL_DOWNLOAD_KEY = "urlDownloadLinux";
#elif defined(__APPLE__)
    const char* URL_DOWNLOAD_KEY = "urlDownloadDarwin";
#elif defined(_WIN32) || defined(_WIN64)
    const char* URL_DOWNLOAD_KEY = "urlDownloadWindows";
#else
    // On unsupported platforms, this key never matches anything, effectively filtering all plugins.
    const char* URL_DOWNLOAD_KEY = "urlDownloadUnsupported";
#endif

    static std::string ReadHttpsUrl(const std::string& url, const std::string& outFile = "")
    {
        std::string host, port, path;
        utility::ParseHostPortPath(url, host, port, path);
        return utility::ReadHttps(host, port, path, outFile);
    }

    struct PluginRegistry::Impl
    {
        explicit Impl(std::shared_ptr<agents> agentsInstance);

        void        fetch_and_parse();
        void        parse_from_cache();
        void        parse_aggregated_toml(const std::string& aggregatedToml);
        void        merge_installed();
        std::string get_cache_content();
        void        write_cache(const std::string& content);

        std::shared_ptr<agents>           _agents;
        std::map<std::string, PluginSpec> _table;
        std::string                       _errorMessage;
        static constexpr std::string_view cacheFileName = {"plugin_registry_cache.toml"};
    };

    PluginRegistry::Impl::Impl(std::shared_ptr<agents> agentsInstance)
        : _agents(std::move(agentsInstance)) {}

    PluginRegistry::PluginRegistry(const std::shared_ptr<agents>& agentsInstance)
        : _impl{std::make_unique<Impl>(agentsInstance)}
    {
        _impl->fetch_and_parse();
    }

    PluginRegistry::~PluginRegistry() = default;

    void PluginRegistry::Impl::fetch_and_parse()
    {
        CBEAM_LOG_DEBUG("Fetching plugin registry from " + REGISTRY_URL);
        std::stringstream aggregatedPluginsTomlForCache;

        try
        {
            const std::string indexToml = ReadHttpsUrl(REGISTRY_URL);

            toml::parse_result result = toml::parse(indexToml);
            if (!result)
            {
                throw std::runtime_error("Failed to parse plugin index TOML: " + std::string(result.error().description()));
            }
            toml::table indexTbl = std::move(result).table();

            toml::array* pluginUrls = indexTbl.get_as<toml::array>("plugin");
            if (!pluginUrls)
            {
                throw std::runtime_error("Plugin index TOML is invalid: missing 'plugin' array.");
            }

            for (auto&& elem : *pluginUrls)
            {
                if (auto urlNode = elem.as_table()->get("url"))
                {
                    if (auto url = urlNode->value<std::string>())
                    {
                        try
                        {
                            std::string        pluginTomlStr = ReadHttpsUrl(*url);
                            toml::parse_result pluginResult  = toml::parse(pluginTomlStr);
                            if (!pluginResult) continue;
                            toml::table pluginTbl = std::move(pluginResult).table();

                            std::string downloadUrl = pluginTbl[URL_DOWNLOAD_KEY].value_or<std::string>("");
                            if (downloadUrl.empty())
                            {
                                CBEAM_LOG_DEBUG("Skipping plugin '" + pluginTbl["displayName"].value_or<std::string>("unknown") + "' as it's not available for this platform.");
                                continue;
                            }

                            PluginSpec spec(pluginTomlStr, downloadUrl);
                            _table.insert_or_assign(spec.GetName(), std::move(spec));

                            aggregatedPluginsTomlForCache << "[[plugin]]\n"
                                                          << pluginTomlStr << "\n";
                        }
                        catch (const std::exception& ex)
                        {
                            CBEAM_LOG("Failed to fetch or process plugin spec from URL '" + *url + "': " + ex.what());
                        }
                    }
                }
            }
            write_cache(aggregatedPluginsTomlForCache.str());
        }
        catch (const std::exception& ex)
        {
            std::string msg = std::string("Connection to the plugin registry could not be established: ") + ex.what();
            CBEAM_LOG(msg);

            try
            {
                parse_from_cache();
                CBEAM_LOG("Using cached plugins list");
                _errorMessage = "Could not connect to online plugin registry - showing cached information.";
            }
            catch (const std::exception&)
            {
                _errorMessage = "Could not connect to online plugin registry and no cache is available.";
            }
        }

        merge_installed();
    }

    void PluginRegistry::Impl::parse_from_cache()
    {
        std::string cachedToml = get_cache_content();
        parse_aggregated_toml(cachedToml);
    }

    void PluginRegistry::Impl::parse_aggregated_toml(const std::string& aggregatedToml)
    {
        if (aggregatedToml.empty()) return;

        toml::parse_result result = toml::parse(aggregatedToml);
        if (!result)
        {
            CBEAM_LOG("Could not parse aggregated plugin TOML from cache.");
            return;
        }
        toml::table tbl = std::move(result).table();

        toml::array* plugins = tbl.get_as<toml::array>("plugin");
        if (!plugins) return;

        for (auto&& elem : *plugins)
        {
            if (!elem.is_table()) continue;
            auto* pluginTbl = elem.as_table();

            // Apply the same filtering logic to the cached data.
            std::string downloadUrl = pluginTbl->get(URL_DOWNLOAD_KEY)->value_or<std::string>("");
            if (downloadUrl.empty())
            {
                continue; // Skip plugin if not for this platform.
            }

            std::stringstream ss;
            ss << *pluginTbl;

            try
            {
                PluginSpec spec(ss.str(), downloadUrl);
                _table.insert_or_assign(spec.GetName(), std::move(spec));
            }
            catch (const std::exception& ex)
            {
                CBEAM_LOG(std::string("Ignored a cached plugin entry. Reason: ") + ex.what());
            }
        }
    }

    void PluginRegistry::Impl::merge_installed()
    {
        for (const auto& installed_plugin : _agents->GetPlugins())
        {
            std::string name   = installed_plugin.first;
            const auto* plugin = dynamic_cast<const AgentPlugin*>(installed_plugin.second.get());

            if (plugin)
            {
                if (_table.count(name) == 1)
                {
                    _table.at(name).SetVersionInstalled(plugin->GetVersionInstalled());
                }
                else
                {
                    _table.insert_or_assign(plugin->GetName(), plugin->GetPluginSpec());
                }
            }
        }
    }

    std::string PluginRegistry::Impl::get_cache_content()
    {
        auto cachePath = cbeam::filesystem::get_user_data_dir() / description::GetProductName() / cacheFileName;
        return cbeam::filesystem::read_file(cachePath);
    }

    void PluginRegistry::Impl::write_cache(const std::string& content)
    {
        auto cachePath = cbeam::filesystem::get_user_data_dir() / description::GetProductName() / cacheFileName;
        cbeam::filesystem::write_file(cachePath, content);
    }

    std::string PluginRegistry::GetErrorMessage()
    {
        return _impl->_errorMessage;
    }

    std::shared_ptr<Agent> PluginRegistry::Get(const std::string& agentName)
    {
        auto it = _impl->_table.find(agentName);
        if (it == _impl->_table.end())
        {
            return nullptr;
        }
        return std::make_shared<AgentPlugin>(_impl->_agents, it->second);
    }

    std::size_t PluginRegistry::Count() const
    {
        return _impl->_table.size();
    }

    void PluginRegistry::RescanInstalled()
    {
        _impl->merge_installed();
    }

    PluginInstallResult PluginRegistry::Install(std::string name, std::string& strError)
    {
        if (_impl->_table.count(name) == 0)
        {
            strError = "Internal error: plugin '" + name + "' is not contained in the list of plugins.";
            return PluginInstallResult::ERROR_WHILE_CREATING_INSTANCE;
        }

        const PluginSpec& spec = _impl->_table.at(name);

        if (spec.GetUrlDownload().empty())
        {
            strError = "Plugin '" + name + "' has no download URL for the current platform.";
            return PluginInstallResult::ERROR_WHILE_CREATING_INSTANCE;
        }

        try
        {
            std::filesystem::path temp_dir = cbeam::filesystem::create_unique_temp_dir();
            std::filesystem::path zip_path = temp_dir / "plugin.zip";

            CBEAM_LOG("Downloading plugin '" + name + "' from " + spec.GetUrlDownload());
            ReadHttpsUrl(spec.GetUrlDownload(), zip_path.string());

            std::filesystem::path extract_dir = temp_dir / "extracted";
            if (::nexuslua::utility::Unzip(zip_path.string(), extract_dir))
            {
                std::filesystem::remove(zip_path);
            }

            return _impl->_agents->InstallPlugin(std::make_shared<AgentPlugin>(_impl->_agents, spec), extract_dir / "archive", strError);
        }
        catch (const std::exception& ex)
        {
            strError = "An error occurred during download or installation: " + std::string(ex.what());
            return PluginInstallResult::ERROR_WHILE_CREATING_INSTANCE;
        }
    }

    // --- Iterator implementation ---

    PluginRegistry::Iterator PluginRegistry::begin()
    {
        return PluginRegistry::Iterator({_impl.get(), 0});
    }

    PluginRegistry::Iterator PluginRegistry::end()
    {
        return PluginRegistry::Iterator({_impl.get(), Count()});
    }

    PluginRegistry::Iterator::value_type PluginRegistry::Iterator::operator*() const
    {
        auto it = m_ptr.impl->_table.begin();
        std::advance(it, m_ptr.pos);

        // This creates a temporary AgentPlugin on the fly.
        return std::make_shared<AgentPlugin>(m_ptr.impl->_agents, it->second);
    }
}