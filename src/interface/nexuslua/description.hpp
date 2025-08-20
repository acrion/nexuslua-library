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

#include "nexuslua_export.h"

#include <string>

namespace nexuslua::description
{
    NEXUSLUA_EXPORT std::string GetOrganizationName();      ///< returns "acrion"
    NEXUSLUA_EXPORT std::string GetProductName();           ///< returns "nexuslua"
    NEXUSLUA_EXPORT std::string GetEmail();                 ///< returns email of current maintainer of nexuslua
    NEXUSLUA_EXPORT std::string GetCopyright();             ///< returns nexuslua copyright
    NEXUSLUA_EXPORT std::string GetVersion();               ///< returns the nexuslua version as set in the CMakeLists.txt
    NEXUSLUA_EXPORT std::string GetLicenseURL();            ///< returns the URL of the nexuslua license
    NEXUSLUA_EXPORT std::string GetLuaVersion();            ///< returns std::string(LUA_VERSION_MAJOR) + "." + LUA_VERSION_RELEASE + "." + LUA_VERSION_MINOR
    NEXUSLUA_EXPORT std::string GetLuaLicenseURL();         ///< returns the URL of the lua license
    NEXUSLUA_EXPORT std::string GetBoostVersion();          ///< returns BOOST_LIB_VERSION, with _ characters replaced by .
    NEXUSLUA_EXPORT std::string GetBoostLicenseURL();       ///< returns the URL of the boost license
    NEXUSLUA_EXPORT std::string GetOpenSSLVersion();        ///< returns OpenSSL_version(OPENSSL_VERSION)
    NEXUSLUA_EXPORT std::string GetOpenSSLLicenseURL();     ///< returns the URL of the OpenSSL license
    NEXUSLUA_EXPORT std::string GetLibZipVersion();         ///< returns LIBZIP_VERSION
    NEXUSLUA_EXPORT std::string GetLibZipLicenseURL();      ///< returns the URL of the LibZip license
    NEXUSLUA_EXPORT std::string GetTomlplusplusVersion();   ///< returns the tomlplusplus version string
    NEXUSLUA_EXPORT std::string GetTomlplusplusLicenseURL();///< returns the URL of the tomlplusplus license
    NEXUSLUA_EXPORT std::string GetVersionsAndLicenses();   ///< return a list of all used
}
