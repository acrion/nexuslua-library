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

#include <filesystem>
#include <functional>
#include <string>

namespace nexuslua
{
    /// \brief used by utils::ReadHttp to call a function to describe the download state, including a string with further information
    enum class DownloadProgress
    {
        CONNECTING,
        SENDING_REQUEST,
        READING_HTTP_RESPONSE,
        READING_DATA,
        DOWNLOADING, ///< string will contain current download size
        ERR,         ///< string will contain error message
        ABORTED
    };

    /// \brief various helper functions
    namespace utility
    {
        /// \brief reads a file from a the given URL (no https)
        /// \param URL the URL to read from
        /// \param outFile an optional path to an output file
        /// \param progress an optional callback function that will be called to set the progress during the download
        /// \param abort an optional callback function that will be checked during download; if it returns true, the download will be canceled
        /// \returns the downloaded string, if outFile is empty; otherwise the empty string
        NEXUSLUA_EXPORT std::string ReadHttp(const std::string& URL, const std::string& outFile = "", std::function<void(DownloadProgress, std::string)> progress = nullptr, std::function<bool()> abort = nullptr);

        /// \brief reads a file from a the given URL (no https)
        /// \param host the host to read from
        /// \param port the port to read from
        /// \param path the path on the host to read from
        /// \param outFile an optional path to an output file
        /// \param progress an optional callback function that will be called to set the progress during the download
        /// \param abort an optional callback function that will be checked during download; if it returns true, the download will be canceled
        /// \returns the downloaded string, if outFile is empty; otherwise the empty string
        NEXUSLUA_EXPORT std::string ReadHttp(const std::string& host, const std::string& port, const std::string& path, const std::string& outFile = "", std::function<void(DownloadProgress, std::string)> progress = nullptr, std::function<bool()> abort = nullptr);

        /// \brief reads a file from a the given URL
        /// \param host the host to read from
        /// \param port the port to read from
        /// \param path the path on the host to read from
        /// \param outFile an optional path to an output file
        /// \param progress an optional callback function that will be called to set the progress during the download
        /// \param abort an optional callback function that will be checked during download; if it returns true, the download will be canceled
        /// \returns the downloaded string, if outFile is empty; otherwise the empty string
        NEXUSLUA_EXPORT std::string ReadHttps(const std::string& host, const std::string& port, const std::string& path, const std::string& outFile = "", std::function<void(DownloadProgress, std::string)> progress = nullptr, std::function<bool()> abort = nullptr);

        NEXUSLUA_EXPORT std::string RemoveWsFromParams(std::string signature);                                                   ///< removes redundant white spaces from the given function signature
        NEXUSLUA_EXPORT void        ParseHostPortPath(std::string URL, std::string& host, std::string& port, std::string& path); ///< splits up the given URL into the three components host, port and path and sets the reference parameters to these
        NEXUSLUA_EXPORT bool        Unzip(std::filesystem::path zip_file, std::filesystem::path target_dir);                     ///< unzips the given zip file to the given target dir. Throws std::runtime_error in case of errors.
        NEXUSLUA_EXPORT bool        Zip(std::filesystem::path source_dir, std::filesystem::path zip_file);                       ///< zips the given target directory to the given output zip file. Throws std::runtime_error in case of errors.
    }
}
