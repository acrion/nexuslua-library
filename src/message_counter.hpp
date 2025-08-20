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

#include <cbeam/lifecycle/singleton.hpp>

#include "nexuslua_export.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace nexuslua
{
    class message_counter
    {
    public:
        static std::shared_ptr<message_counter> get()
        {
            return cbeam::lifecycle::singleton<message_counter>::get("nexuslua::message_counter");
        }

        inline void wait_until_first()
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if (!_increase_was_called)
            {
                _cv.wait(lock, [this]
                         { return _increase_was_called.load(); });
            }
        }

        inline void wait_until_empty()
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if (_size > 0)
            {
                _cv.wait(lock, [this]
                         { return _size == 0; });
            }
        }

        inline int64_t size()
        {
            return _size;
        }

        inline void increase()
        {
            std::lock_guard lock(_mtx);
            if (++_size == 1)
            {
                _increase_was_called = true;
                CBEAM_LOG_DEBUG("message_counter::increase: notifying.");
                _cv.notify_one();
            }
        }

        inline void decrease()
        {
            std::lock_guard lock(_mtx);
            if (--_size == 0)
            {
                CBEAM_LOG_DEBUG("message_counter::Decrease: notifying.");
                _cv.notify_one();
            }
        }

    private:
        std::atomic<bool>       _increase_was_called{false};
        std::atomic<int64_t>    _size{0};
        std::mutex              _mtx;
        std::condition_variable _cv;
    };
} // namespace nexuslua
