**A C++ library for embedding a multi-threaded, message-passing Lua environment.**

`nexuslua` is a C++ library that extends Lua with a powerful concurrency model based on **agents** and **asynchronous
messaging**. It allows you to run multiple Lua states - and even C++ components - in separate OS-level threads, communicating safely and efficiently through a message-passing architecture. This solves common problems in complex applications, such as UI blocking from long-running tasks and tangled state management.

nexuslua integrates with your C++ project by extending the **official, unmodified Lua 5.4 core**. This means you get powerful new concurrency features without sacrificing compatibility with the existing Lua ecosystem. No surprises!

The library is designed to be lightweight and embeddable, staying true to the spirit of Lua. While it currently uses some Boost components, the long-term goal is to replace all boost dependencies with [Cbeam](https://github.com/acrion/Cbeam).

> **Watch the Talk:** For a deep dive into the original concepts, check out the presentation from the 
[Lua Workshop 2022](https://www.youtube.com/watch?v=Y6wz6Qk475E) (under the project's former name, "acrionlua").
>
> Looking for a user-focused introduction?
For a hands-on tutorial on how to use agents and plugins, please see the [nexuslua executable README
](https://github.com/acrion/nexuslua).

---

### A Modern Concurrency Model for C++ and Lua

You might be wondering how `nexuslua` can be "lightweight" with dependencies like Boost. That's true, `nexuslua` is no longer lightweight in the sense of a single Lua header file. However, its design philosophy is to be **lightweight in the context of modern C++ development**. The dependency on Boost is also part of a clear roadmap: the long-term goal is to replace all Boost components with [Cbeam](https://github.com/acrion/Cbeam), a purpose-built, header-only library that already powers the core messaging system.

While `nexuslua` can be added to any C++ project, it offers a particularly powerful upgrade path for applications that **already use Lua**. Its key strength lies in bridging the gap between high-performance C++ and the flexible Lua scripting environment.

#### The Upgrade Path

1. **Seamless Adoption**: For projects already embedding Lua, you can switch to `nexuslua` as your interpreter. Your existing Lua code and C++ bindings will continue to work as before, as the Lua 5.4 core remains **unmodified**. This provides a stable, zero-friction starting point.
2. **Gradual Modernization**: From this baseline, you can start leveraging `nexuslua`'s true power. Instead of maintaining complex, tightly-coupled C++/Lua bindings, you can gradually refactor your architecture towards a clean, asynchronous messaging model.
3. **True Parallelism**: The ultimate goal is a decoupled system where your C++ components (as native Cbeam agents) and your Lua scripts (as `nexuslua` agents) communicate seamlessly and safely across different OS threads. This is ideal for offloading long-running tasks from a main thread (e.g., in a GUI application) or parallelizing heavy computations.

A key advantage of this approach is that `nexuslua` is an extension, not a fork. This means:

- **Stability and Trust**: You are running the official, battle-tested Lua interpreter. *Pas de surprise!* (No surprises!).
- **Compatibility**: Existing C/C++ libraries that bind to Lua and pure Lua modules will work without modification.

### Effortless Integration with CMake

True to its goal of being lightweight for C++ developers, `nexuslua` is designed to be integrated into your project seamlessly using modern CMake's `FetchContent` module.

Just add the following to your `CMakeLists.txt`:

```cmake
# In your project's CMakeLists.txt

include(FetchContent)

# Declare nexuslua as a dependency from GitHub
FetchContent_Declare(
    nexuslua
    GIT_REPOSITORY https://github.com/acrion/nexuslua.git
    GIT_TAG v0.9.0 # Use the desired version tag
)

# Make the dependency available to your project
FetchContent_MakeAvailable(nexuslua)

# ... now define your own executable or library
add_executable(my_awesome_app main.cpp)

# Link against nexuslua - CMake handles the rest (headers, dependencies, etc.)
target_link_libraries(my_awesome_app PRIVATE nexuslua::nexuslua)
```

That's it. CMake will automatically fetch the specified version of `nexuslua`, configure it, and make the `nexuslua::nexuslua` target available for you to link against. All transitive dependencies like Lua are handled automatically.

---

## Core Architecture

nexuslua's design revolves around three main concepts:

1. **Agents**: An agent is an independent entity that runs in its own OS thread. It has a name and a set of message handlers. Agents can be written in Lua (
   `AgentLua`) or C++ (`AgentCpp`). A special type, `AgentPlugin`, adds metadata for discovery and management.
2. **Messages**: Agents communicate exclusively by sending asynchronous messages to each other. A message consists of a name and a parameter payload, which is a flexible, nested key-value structure (
   `nexuslua::LuaTable`). The sender does not block; it fires the message and continues its work.
3. **The `nexuslua::agents` Manager**: This is the central C++ class that you, as the embedder, interact with. It manages the lifecycle of all agents, dispatches messages between them, and provides the entry point into the nexuslua ecosystem.

---

## Key Features

- **True Parallelism**: Execute Lua scripts and C++ code in concurrent OS threads.
- **Asynchronous Messaging**: Decouple your application components for a more robust and responsive design.
- **Dynamic C/C++ Library Loading**: The
  `import()` function allows Lua scripts to call functions from native shared libraries (`.dll`, `.so`,
  `.dylib`) without boilerplate binding code.
- **Plugin System**: Build modular, installable components (
  `AgentPlugin`) with metadata for versioning, licensing, and discovery via an online marketplace.
- **Safe Memory Management**: Includes mechanisms like
  `cbeam::stable_reference_buffer` to safely manage memory lifetimes across the C++/Lua boundary, especially for pointers passed in messages.

## Embedding nexuslua: C++ API Quickstart

Here’s how to create a simple multi-agent system in your C++ application.

```cpp
#include "nexuslua/agents.hpp"
#include "nexuslua/message.hpp"
#include <iostream>
#include <thread>

int main() {
    // 1. Create the central agent manager
    auto agents = std::make_shared<nexuslua::agents>();

    // 2. Define a C++ agent handler
    agents->Add("cpp_handler", [](const std::shared_ptr<nexuslua::Message>& msg) {
        std::cout << "C++ agent received message: " << msg->name << std::endl;
        // Process the message...
    });
    agents->AddMessageForCppAgent("cpp_handler", "ping");

    // 3. Create a Lua agent from a script file
    agents->Add("lua_agent", "path/to/my_agent.lua");

    // Let the agents start up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 4. Send a message from C++ to the Lua agent
    nexuslua::LuaTable params;
    params.data["value"] = 42;
    agents->GetMessage("lua_agent", "process_data").Send(params);
    
    // Wait for all asynchronous work to complete before exiting
    agents->WaitUntilMessageQueueIsEmpty();
    agents->ShutdownAgents();
    
    return 0;
}
```

In `my_agent.lua`:

```lua
function process_data(parameters)
    log("Lua agent received value: " .. tostring(parameters.value))
    -- Send a message back to the C++ agent
    send("cpp_handler", "ping", {})
end

addmessage("process_data")
```

## Extended Lua API

nexuslua provides a set of powerful new global functions to Lua scripts.

| Function                           | Description                                                      |
|:-----------------------------------|:-----------------------------------------------------------------|
| `addagent(name, code, [messages])` | Creates a new agent running in a separate thread.                |
| `addmessage(name, [metadata])`     | Registers a function as a message handler for the current agent. |
| `send(agent, message, params)`     | Sends an asynchronous message to another agent.                  |
| `import(lib, func, signature)`     | Loads a function from a C/C++ shared library.                    |
| `isreplicated()`                   | Checks if the current script is a replicated instance.           |
| `cores()`                          | Returns the number of available hardware threads.                |
| `time()`                           | High-resolution timer for benchmarking.                          |
| ... and more                       | `readfile`, `zip`, `unzip`, `env`, `log`, etc.                   |

For a complete list and detailed documentation of all available functions, please visit **[https://nexuslua.org](https://nexuslua.org)**.

### Calling Native Code with `import()`

The
`import()` function is a cornerstone of nexuslua, making it easy to offload performance-critical work to native code.

**In Lua:**

```lua
-- Load 'MyFunction' from 'my_native_lib'
-- Signature: takes a table, returns a table
import("my_native_lib", "MyFunction", "table(table)")

local params = { value = 100 }
local result = AddFourtyTwo(params) -- This is a blocking call

-- Result will be a lua table with key 'example' and value '142'.
```

**In C++ (`my_native_lib.cpp`):**

```cpp
#include "cbeam/serialization/nested_map.hpp"
#include "cbeam/container/stable_reference_buffer.hpp"

using namespace cbeam::serialization;
using namespace cbeam::container;

// The function signature MUST use extern "C" for stable ABI
extern "C" NEXUSLUA_EXPORT nested_map AddFourtyTwo(void* serialized_lua_table) {
    nested_map params = deserialize(serialized_lua_table);
    int val = params['value'];
    
    nested_map result;
    result['example']= val+42;
    
    auto buffer = serialize(result);
    return buffer.get();
}
```

> **Memory Management Note**: When the shared library needs to allocate memory and return it to back to Lua,
> it must be allocated with `cbeam::stable_reference_buffer` with one of these constructors:
>
> ```
> stable_reference_buffer(const std::size_t size, const size_t size_of_type = 1); // create a managed memory block of the given size
> explicit stable_reference_buffer(const void* address);                          // create a managed memory block from preallocated address
> ```
>
> This ensures the memory remains valid even after the shared library is unloaded and the only reference is held by the Lua garbage collector.

---

## Community

Have questions, want to share what you've built, or just say hi? Join our community on Discord!

💬 **[Join the nexuslua Discord Server](https://discord.gg/aaZPevdEen)**

---

**Dependencies:**

```bash
❯ nexuslua -v
nexuslua 0.9.0     https://github.com/acrion/nexuslua/blob/main/LICENSE
Lua 5.4.6          https://www.lua.org/license.html
Cbeam 0.9.4        https://www.cbeam.org/license.html
Boost 1.83         https://www.boost.org/users/license.html
tomlplusplus 3.4.0 https://github.com/marzer/tomlplusplus/blob/master/LICENSE
libzip 1.11.4      https://github.com/nih-at/libzip/blob/main/LICENSE
OpenSSL 3.5.2      https://www.openssl.org/source/license.html
```
