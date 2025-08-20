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

#include "description.hpp"

#include "config.hpp"
#include "lua.hpp"
#include "platform_specific.hpp"

#include <cbeam/platform/system_folders.hpp>
#include <cbeam/serialization/xpod.hpp>
#include <cbeam/version.hpp>

#include "version_nexuslua.hpp"

CBEAM_SUPPRESS_WARNINGS_PUSH()
#if NEXUSLUA_WITH_OPENSSL
    #include <openssl/opensslv.h>
#endif

#include "zipconf.h"
#include <toml++/toml.h>

#include <boost/tokenizer.hpp>

CBEAM_SUPPRESS_WARNINGS_POP()

using namespace std::string_literals;

namespace nexuslua::description
{
    std::string GetOrganizationName()
    {
        return "acrion innovations GmbH";
    }

    std::string GetProductName()
    {
        return "nexuslua";
    }

    std::string GetEmail()
    {
        return "s.zipproth@acrion.ch";
    }

    std::string GetCopyright()
    {
        return GetProductName() + " " + NEXUSLUA_VERSION + "  Copyright (C) 2025 Stefan Zipproth";
    }

    std::string GetVersion()
    {
        return NEXUSLUA_VERSION;
    }

    std::string GetLicenseURL()
    {
        return "https://github.com/acrion/nexuslua/blob/main/LICENSE";
    }

    std::string GetCbeamVersion()
    {
        return cbeam::get_version();
    }

    std::string GetCbeamLicenseURL()
    {
        return "https://www.cbeam.org/license.html";
    }

    std::string GetLuaVersion()
    {
        return Lua::GetVersion();
    }

    std::string GetLuaLicenseURL()
    {
        return "https://www.lua.org/license.html";
    }

    std::string GetBoostVersion()
    {
        std::string version = BOOST_LIB_VERSION;
        std::replace(version.begin(), version.end(), '_', '.');
        return version;
    }

    std::string GetBoostLicenseURL()
    {
        return "https://www.boost.org/users/license.html";
    }

    std::string GetOpenSSLVersion()
    {
#if NEXUSLUA_WITH_OPENSSL
        return OPENSSL_FULL_VERSION_STR;
#else
        return "(compiled without OpenSSL)";
#endif
    }

    std::string GetOpenSSLLicenseURL()
    {
        return "https://www.openssl.org/source/license.html";
    }

    std::string GetLibZipVersion()
    {
        return LIBZIP_VERSION;
    }

    std::string GetLibZipLicenseURL()
    {
        return "https://github.com/nih-at/libzip/blob/main/LICENSE";
    }

    std::string GetTomlplusplusVersion()
    {
        return std::to_string(TOML_LIB_MAJOR) + "." +
               std::to_string(TOML_LIB_MINOR) + "." +
               std::to_string(TOML_LIB_PATCH);
    }

    std::string GetTomlplusplusLicenseURL()
    {
        return "https://github.com/marzer/tomlplusplus/blob/master/LICENSE";
    }

    std::string GetVersionsAndLicenses()
    {
        std::vector<std::tuple<std::string, std::string, std::string>> items = {
            {GetProductName(), GetVersion(), GetLicenseURL()},
            {"Lua", GetLuaVersion(), GetLuaLicenseURL()},
            {"Cbeam", GetCbeamVersion(), GetCbeamLicenseURL()},
            {"Boost", GetBoostVersion(), GetBoostLicenseURL()},
            {"tomlplusplus", GetTomlplusplusVersion(), GetTomlplusplusLicenseURL()},
            {"libzip", GetLibZipVersion(), GetLibZipLicenseURL()},
            {"OpenSSL", GetOpenSSLVersion(), GetOpenSSLLicenseURL()}};

        size_t maxFirstPartWidth = 0;
        for (const auto& item : items)
        {
            size_t combinedWidth = std::get<0>(item).size() + 1 + std::get<1>(item).size(); // +1 für das Leerzeichen zwischen Name und Version
            if (combinedWidth > maxFirstPartWidth)
            {
                maxFirstPartWidth = combinedWidth;
            }
        }

        std::stringstream result;
        for (const auto& item : items)
        {
            result << std::left << std::setw(maxFirstPartWidth)
                   << std::get<0>(item) + " " + std::get<1>(item)
                   << " " << std::get<2>(item) << std::endl;
        }

        std::string finalResult = result.str();
        if (!finalResult.empty())
        {
            finalResult.pop_back();
        }

        return finalResult;
    }
}
