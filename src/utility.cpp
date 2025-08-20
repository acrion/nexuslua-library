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

#include "utility.hpp"

#include "config.hpp"
#include "description.hpp"
#include "platform_specific.hpp"

#include <cbeam/logging/log_manager.hpp>

#include "zip.h"

CBEAM_SUPPRESS_WARNINGS_PUSH()
#include <utility> // to provide std::exchange for boost/asio/awaitable.hpp:69

#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#if NEXUSLUA_WITH_OPENSSL
    #include <boost/asio/ssl/error.hpp>
    #include <boost/asio/ssl/rfc2818_verification.hpp>
    #include <boost/asio/ssl/stream.hpp>
    #include <boost/beast/core.hpp>
    #include <boost/beast/http.hpp>
    #include <boost/beast/version.hpp>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/tokenizer.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
CBEAM_SUPPRESS_WARNINGS_POP()

#include <fstream>
#include <memory>
#include <regex>
#include <sstream>

namespace nexuslua::utility
{
    std::string ReadHttp(const std::string& URL, const std::string& outFile, std::function<void(DownloadProgress, std::string)> progress, std::function<bool()> abort)
    {
        std::string host;
        std::string port;
        std::string path;
        ParseHostPortPath(URL, host, port, path);
        return ReadHttp(host, port, path, outFile, progress, abort);
    }

    std::string ReadHttp(const std::string& host, const std::string& port, const std::string& path, const std::string& outFile, std::function<void(DownloadProgress, std::string)> progress, std::function<bool()> abort)
    {
        if (port == "443")
        {
#if NEXUSLUA_WITH_OPENSSL
            return ReadHttps(host, port, path, outFile, progress, abort);
#else
            throw std::runtime_error("Error: This nexuslua binary has been compiled without SSL support (CMake option NEXUSLUA_WITH_OPENSSL).");
#endif
        }

        try
        {
            if (progress)
                progress(DownloadProgress::CONNECTING, "Connecting...");

            using boost::asio::ip::tcp;
            boost::asio::io_service io_service;

            tcp::resolver           resolver(io_service);
            tcp::resolver::query    query(host, port);
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

            tcp::socket socket(io_service);
            boost::asio::connect(socket, endpoint_iterator);

            boost::asio::streambuf request;
            std::ostream           request_stream(&request);
            request_stream << "GET " << path << " HTTP/1.0\r\n";
            request_stream << "Host: " << host << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Connection: close\r\n\r\n";

            if (abort && abort())
            {
                if (progress)
                    progress(DownloadProgress::ABORTED, "Aborted");
                return "";
            }
            if (progress)
                progress(DownloadProgress::SENDING_REQUEST, "Sending request...");

            boost::asio::write(socket, request);

            if (abort && abort())
            {
                if (progress)
                    progress(DownloadProgress::ABORTED, "Aborted");
                return "";
            }
            if (progress)
                progress(DownloadProgress::SENDING_REQUEST, "Reading HTTP response...");

            boost::asio::streambuf response;
            boost::asio::read_until(socket, response, "\r\n");

            std::istream response_stream(&response);
            std::string  http_version;
            response_stream >> http_version;
            unsigned int status_code;
            response_stream >> status_code;
            std::string status_message;
            std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/")
            {
                throw std::runtime_error("Invalid response");
            }
            if (status_code != 200)
            {
                throw std::runtime_error("Response returned with status code " + std::to_string(status_code));
            }

            boost::asio::read_until(socket, response, "\r\n\r\n");

            std::stringstream headerStream;
            std::string       header;
            while (std::getline(response_stream, header) && header != "\r")
                headerStream << header << "\n";

            std::ostream* s = outFile.empty() ? dynamic_cast<std::ostream*>(new std::stringstream())
                                              : new std::ofstream(outFile, std::ofstream::binary);

            if (response.size() > 0)
                *s << &response;

            if (abort && abort())
            {
                if (progress)
                    progress(DownloadProgress::ABORTED, "Aborted");
                return "";
            }
            if (progress)
                progress(DownloadProgress::DOWNLOADING, "0 bytes");

            boost::system::error_code error;
            std::size_t               total = 0;
            while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
            {
                total += response.size();

                if (abort && abort())
                {
                    if (progress)
                        progress(DownloadProgress::ABORTED, "Aborted");
                    return "";
                }
                if (progress)
                {
                    if (total < 1024)
                    {
                        progress(DownloadProgress::DOWNLOADING, std::to_string(total) + " bytes");
                    }
                    else if (total < 1024 * 1024)
                    {
                        progress(DownloadProgress::DOWNLOADING, std::to_string(total / 1024) + " KB");
                    }
                    else
                    {
                        progress(DownloadProgress::DOWNLOADING, std::to_string(total / (int)(1024 * 102.4) / 10.0) + " MB");
                    }
                }

                *s << &response;
            }

            if (error != boost::asio::error::eof)
            {
                throw boost::system::system_error(error);
            }

            s->flush();
            std::string result;
            if (outFile.empty())
            {
                result = dynamic_cast<std::stringstream*>(s)->str();
            }

            delete s;
            return result;
        }
        catch (std::exception& ex)
        {
            if (progress)
                progress(DownloadProgress::ERR, ex.what());
            throw;
        }
    }
    std::string ReadHttps(const std::string& host, const std::string& port, const std::string& target, const std::string& outFile, std::function<void(DownloadProgress, std::string)> progress, std::function<bool()> abort)
    {
#if NEXUSLUA_WITH_OPENSSL
        namespace beast = boost::beast;
        namespace http  = beast::http;
        namespace net   = boost::asio;
        namespace ssl   = net::ssl;
        using tcp       = net::ip::tcp;

        try
        {
            if (progress)
            {
                progress(DownloadProgress::CONNECTING, "Connecting...");
            }

            int version = 11; // HTTP/1.1

            net::io_context ioc;
            ssl::context    ctx{ssl::context::sslv23_client};

            tcp::resolver            resolver{ioc};
            ssl::stream<tcp::socket> stream{ioc, ctx};

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            auto const results = resolver.resolve(host, port);

            net::connect(stream.next_layer(), results.begin(), results.end());
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::get, target, version};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            if (progress)
            {
                progress(DownloadProgress::SENDING_REQUEST, "Reading HTTP response...");
            }

            http::write(stream, req);

            beast::flat_buffer                 buffer;
            http::response<http::dynamic_body> res;

            if (progress)
            {
                progress(DownloadProgress::DOWNLOADING, "");
            }

            http::read(stream, buffer, res);

            if (res.result() != boost::beast::http::status::ok)
            {
                std::ostringstream ss;
                ss << res.result();
                throw std::runtime_error(ss.str());
            }

            std::ostream* s = outFile.empty() ? dynamic_cast<std::ostream*>(new std::stringstream())
                                              : new std::ofstream(outFile, std::ofstream::binary);

            std::string result;
            size_t      len;
            {
                const auto str = beast::buffers_to_string(res.body().data());
                *s << str;
                s->flush();
                len = str.length();

                if (outFile.empty())
                {
                    result = str;
                }
            }

            delete s;

            beast::error_code ec;
            stream.shutdown(ec);
            if (ec != net::error::eof && (ec != boost::asio::ssl::error::stream_truncated || len == 0))
            {
                throw beast::system_error{ec};
            }

            return result;
        }
        catch (const std::exception& ex)
        {
            if (progress)
            {
                progress(DownloadProgress::ERR, ex.what());
            }
            throw;
        }
#else
        return ReadHttp(host, port, target, outFile, progress, abort);
#endif
    }

    std::string RemoveWsFromParams(std::string signature)
    {
        typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
        boost::char_separator<char>                           sep("", ",()", boost::keep_empty_tokens);
        tokenizer                                             tokens(signature, sep);
        std::stringstream                                     result;
        for (tokenizer::iterator token = tokens.begin();
             token != tokens.end();
             ++token)
        {
            std::string trimmed_token = *token;
            boost::algorithm::trim(trimmed_token);
            result << trimmed_token;
        }
        return result.str();
    }

    void ParseHostPortPath(std::string URL, std::string& host, std::string& port, std::string& path)
    {
        std::string pattern = R"((https?)://(\w+\.\w+)(:(\d+))?(\S+))";
        std::smatch match;

        if (!std::regex_search(URL, match, std::regex(pattern)))
        {
            throw std::runtime_error("Could not parse URL '" + URL + "'");
        }

        std::vector<std::string> subMatches;
        for (std::string subMatch : match)
        {
            subMatches.emplace_back(subMatch);
        }

        std::string protocol = subMatches[1];
        host                 = subMatches[2];
        port                 = subMatches[4];
        path                 = subMatches[5];

        if (port.empty())
        {
            if (protocol == "https")
                port = "443";
            else if (protocol == "ssh")
                port = "22";
            else
                port = "80";
        }
    }

    bool Unzip(std::filesystem::path zip_file, std::filesystem::path target_dir)
    {
        int         err;
        struct zip* za = zip_open(zip_file.string().c_str(), 0, &err);

        if (!za)
        {
            throw std::runtime_error("Could not open zip file: '" + zip_file.string() + "': " + std::to_string(err));
        }

        std::filesystem::create_directories(target_dir);

        for (int i = 0; i < zip_get_num_entries(za, 0); i++)
        {
            struct zip_stat sb;
            if (zip_stat_index(za, i, 0, &sb) == 0)
            {
                std::filesystem::path target_path = target_dir / sb.name;

                if (target_path.string().back() == '/')
                {
                    std::filesystem::create_directories(target_path);
                }
                else
                {
                    struct zip_file* zf = zip_fopen_index(za, i, 0);
                    if (!zf)
                    {
                        zip_close(za);
                        throw std::runtime_error("Could not extract zip file '" + std::string(sb.name) + "'");
                    }

                    std::filesystem::path parent_path = target_path.parent_path();
                    std::filesystem::create_directories(parent_path);

                    std::ofstream wstream(target_path.string(), std::ios::binary);

                    if (!wstream.good())
                    {
                        zip_fclose(zf);
                        zip_close(za);
                        throw std::runtime_error("During zip extraction, could not open output file '" + target_path.string() + "'");
                    }

                    decltype(sb.size) sum = 0;
                    char              buf[1024];
                    while (sum != sb.size)
                    {
                        zip_int64_t len = zip_fread(zf, buf, sizeof(buf));
                        if (len <= 0)
                        {
                            zip_fclose(zf);
                            zip_close(za);
                            throw std::runtime_error("Error while extracting file '" + std::string(sb.name) + "'  from zip archive");
                        }
                        wstream.write(buf, len);
                        sum += len;
                    }
                    zip_fclose(zf);
                }
            }
        }

        return !zip_close(za);
    }

    bool Zip(std::filesystem::path source_dir, std::filesystem::path zip_file)
    {
        int         err;
        zip_error_t zip_err;
        struct zip* archive = zip_open(zip_file.string().c_str(), ZIP_CREATE | ZIP_EXCL, &err);

        if (!archive)
        {
            throw std::runtime_error("Could not create zip file: '" + zip_file.string() + "': " + std::to_string(err));
        }

        std::function<void(const std::filesystem::path&, const std::string&)> zipDirectory = [&](const std::filesystem::path& dir, const std::string& subdir)
        {
            for (auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    zipDirectory(entry.path(), subdir + entry.path().filename().string() + "/");
                }
                else
                {
                    std::ifstream stream(entry.path(), std::ios::binary);
                    if (!stream.is_open())
                    {
                        zip_close(archive);
                        throw std::runtime_error("Could not open input file: '" + entry.path().string() + "'");
                    }

                    std::string fileName = subdir + entry.path().filename().string();
                    stream.seekg(0, std::ios::end);
                    size_t size = stream.tellg();
                    stream.seekg(0, std::ios::beg);

                    std::vector<char> buffer(size);
                    stream.read(buffer.data(), size);

                    char* dynamic_buffer = new char[size];
                    std::copy(buffer.begin(), buffer.end(), dynamic_buffer);

                    struct zip_source* source = zip_source_buffer_create(dynamic_buffer, size, 1, &zip_err);
                    if (!source)
                    {
                        delete[] dynamic_buffer;
                        zip_close(archive);
                        throw std::runtime_error("Could not create source for file: '" + fileName + "': " + std::to_string(zip_err.zip_err));
                    }

                    if (zip_file_add(archive, fileName.c_str(), source, ZIP_FL_ENC_UTF_8) < 0)
                    {
                        delete[] dynamic_buffer;
                        zip_source_free(source);
                        zip_close(archive);
                        throw std::runtime_error("Error while adding file '" + fileName + "' to zip archive");
                    }
                }
            }
        };

        zipDirectory(source_dir, "");

        return !zip_close(archive);
    }
}
