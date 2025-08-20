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

#include "platform_specific.hpp"

#include "config.hpp"
#include "description.hpp"
#include "utility.hpp"

#include <cbeam/platform/runtime.hpp>
#include <cbeam/logging/log_manager.hpp>

#include <chrono>
#include <clocale>
#include <sstream>

#ifdef _WIN32
    #undef WINVER
    #undef _WIN32_WINNT
    #define WINVER       0x0A00 // 0x0601
    #define _WIN32_WINNT 0x0A00 // 0x0601
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <shlwapi.h>
    #include <windows.h>
    #ifdef _MSC_VER
        #pragma comment(lib, "shlwapi.lib")
    #endif
    #include "shlobj.h"

    #include "roapi.h"
    #pragma comment(lib, "runtimeobject.lib")
#else
    #include "pwd.h"
    #include <sys/types.h>
    #include <unistd.h>
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #include <dlfcn.h>
    #ifdef __linux__
        #include "config.hpp"
    #elif defined(__APPLE__)
        #include <dlfcn.h>
        #include <mach-o/dyld.h>
        #include <mach-o/ldsyms.h>
        #include <mach/mach_time.h>
    #else
        #error Unknown OS
    #endif
#endif

namespace nexuslua
{
    struct PlatformSpecific::Impl
    {
        Impl() = default;
        void init_path_to_runtime_binary();

        std::filesystem::path                    _appDataFolder;
        static std::unique_ptr<PlatformSpecific> _instance;
    };

    std::unique_ptr<PlatformSpecific> PlatformSpecific::Impl::_instance;

    PlatformSpecific::PlatformSpecific()
        : _impl{std::make_unique<Impl>()}
    {
    }

    PlatformSpecific& PlatformSpecific::Get()
    {
        if (!Impl::_instance)
        {
            Impl::_instance = std::unique_ptr<PlatformSpecific>(new PlatformSpecific());
        }
        return *Impl::_instance;
    }

#ifdef _WIN32
    // needed for get_version (GetModuleHandleExA)
    void dllfunction() {}
#endif

    std::string PlatformSpecific::GetInternalVersion() // TODO move to core?
    {
#ifdef __linux__
        // On Linux, versions of so files are not encoded in the binary resources. So we encode it here to adapt to the other platforms.
        return PROJECT_VERSION;
#else
        int v1 = 0;
        int v2 = 0;
        int v3 = 0;

    #ifdef _WIN32
        HMODULE hm = NULL;

        if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&dllfunction,
                &hm))
        {
            char fileName[1024];
            fileName[GetModuleFileNameA(hm, fileName, sizeof(fileName))] = 0;
            DWORD handle                                                 = 0;
            BYTE  versionInfo[1024];
            if (GetFileVersionInfo(fileName, handle, sizeof(versionInfo), versionInfo))
            {
                // we have version information
                UINT              len  = 0;
                VS_FIXEDFILEINFO* vsfi = NULL;
                VerQueryValue(versionInfo, "\\", (void**)&vsfi, &len);
                v1     = HIWORD(vsfi->dwFileVersionMS);
                v2     = LOWORD(vsfi->dwFileVersionMS);
                v3     = HIWORD(vsfi->dwFileVersionLS);
                int v4 = LOWORD(vsfi->dwFileVersionLS);

                if (v4 != 0)
                {
                    throw std::logic_error("4th version number must be 0 for platform independent version!");
                }
            }
        }
    #elif defined(__APPLE__)
        #ifdef __LP64__
        const struct mach_header_64* mh = &_mh_dylib_header;
        struct load_command*         lc = (struct load_command*)((void*)((char*)mh + sizeof(struct mach_header_64)));
        #else
        const struct mach_header* mh = &_mh_dylib_header;
        struct load_command*      lc = (struct load_command*)((void*)((char*)mh + sizeof(struct mach_header)));
        #endif
        uint32_t     i       = 0;
        unsigned int version = -1;

        for (; i < mh->ncmds; i++)
        {
            if (lc->cmd == LC_ID_DYLIB)
            {
                version = ((struct dylib_command*)lc)->dylib.current_version;
                break;
            }
            else
            {
                lc = (struct load_command*)((void*)((char*)lc + lc->cmdsize));
            }
        }
        v1 = version / 65536;
        v2 = (version % 65536) / 256;
        v3 = version % 256;
    #else
        #error Unknown OS
    #endif

        std::stringstream stream;
        stream << v1 << "." << v2 << "." << v3;

        if (stream.str() != std::string(PROJECT_VERSION))
        {
            throw std::runtime_error("Internal versioning inconsistency: '" + cbeam::platform::get_path_to_runtime_binary().string() + "' has version '" + stream.str() + "', but it should be '" + std::string(PROJECT_VERSION) + "'");
        }

        return stream.str();
#endif
    }
}