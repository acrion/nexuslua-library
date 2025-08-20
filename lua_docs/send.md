send {#send}
====

Within nexuslua, the [send](send.md) function serves as a method to dispatch an asynchronous message to an existing agent.
Conceptually, this action can be seen as making an asynchronous function call in Lua.
The primary purpose is to relay a message to an agent, which then executes a specific function asynchronously, and if specified, sends back a response.

Agents can be created in two ways:

- Via C++, see nexuslua::agents::Add(), or
- Through the nexuslua function [addagent](addagent.md).

# Parameters

- Name of the agent to which the message should be dispatched.
- Name of the message.
- Message parameter. This can be any Lua table, but it's limited in that its values must be of the following types:
    - Another Lua table
    - A floating-point number
    - An integer
    - A string. It's crucial to note that strings describing a hexadecimal number might be construed as memory addresses. For more details, refer to [touserdata](touserdata.md).

# Example

```lua
local max_threads = cores()
send("numbers", "IsPrime", {number=127, threads=max_threads, reply_to={agent="main", message="CountPrime"}})
```

Beyond the main message data (`number=127`), this example incorporates two optional message entries:

- "threads": Dictates the max number of OS threads nexuslua will instantiate if the recipient agent is occupied when a new message arrives. The "busy" state is determined using the value of \ref nexuslua::Configuration::luaStartNewThreadTime "luaStartNewThreadTime". Without this field, this automatic *replication* won't be done. Note that through the concept of [agents](addagent.md) you have another way to use distribute messages among OS threads.
- "reply_to": Indicates a subtable to which the receiving message will relay its response. The response message will carry the name provided in `message` and will be dispatched to the agent named in `agent`. If `agent` is not specified, it defaults to the agent that performs the `send`.

When a valid `reply_to` subtable is available (as shown above), the specified callback function is invoked asynchronously, using the function's return table as its argument. The callback function referenced in above `send` example could be:

```lua
function CountPrime(parameters) 
    if parameters.isPrime then
        count = count + 1
    end
end
```

It evaluates the result message from `IsPrime`. See [addmessage](addmessage.md) for the `IsPrime` example function's implementation, which returns either `{isPrime=true}` or `{isPrime=false}`, based on the outcome.

To determine which number was assessed, you can refer to `parameters.original_message` which always encapsulates the entire processed message (i.e., the third argument of the associated [send](send.md) call).
Hence, in this illustration, `parameters.original_message.number` reveals the number to which `parameters.isPrime` pertains.

Internally, the C++ types of the dispatched (or replied, see [example](addmessage.md)) parameter values align with the pertinent Lua types used in the dispatched message.
For `void*`, the internal type is a string.
If a table contains [pointers](https://cbeam.org/doxygen/classcbeam_1_1memory_1_1pointer.html), the conversion between `void*` and its representation is managed by `lua_pushtable` and `lua_totable`,
https://github.com/acrion/nexuslua-library/blob/main/src/lua.hpp.

# Merge functionality for message replies

The receiving function of a message has access to the original message, i. e. the message that the sender had processed, through the sub table `original_message` (see previous section).
In spite of this, there are certain use cases where you want to define fields that the function that processes your message shall add to its reply.
You can do this by adding a sub table named `merge` to the `reply_to` subtable. The `merge` sub table will be added to the actual replying message (potentially overwriting parts of the reply).
For example

```lua
function ReplyTest(parameters)
    local result = "Hello " .. parameters.myName
    return {MyReply=result}
end

function ReceiveReply(parameters)
    print(parameters.MyReply)
    print()
    printtable(parameters)
end

addmessage("ReplyTest")
addmessage("ReceiveReply")

send("main", "ReplyTest", {myName="nexuslua", reply_to={message="ReceiveReply", merge={MergedEntry=42, MergedSubTable={AnotherMergeEntry="test"}}}})
```

This script sends the string `nexuslua` to the function `ReplyTest`, which will prepend `Hello ` to the string and send the resulting string to function `ReceiveReply`.
Hence, its code line `print(parameters.MyReply)` will print `Hello nexuslua`.

When sending the message we specified a sub table `reply_to`/`merge` that contains two fields `MergedEntry` and `MergedSubTable`.
Therefore, the table will contain not only `MyReply`, but also these two fields.
So the code line `printtable(parameters)` will print:

```
MyReply Hello nexuslua
MergedEntry     42
MergedSubTable
                AnotherMergeEntry       test
```

In addition it contains an entry `original_message` with the complete message that `ReplyTest` had received.

# Key Insights

The `send` function is more than just a message dispatcher.
It establishes asynchronous communication between different agents running in separate operating system threads.
This means instead of waiting for a response, you can continue with other tasks and process the response as it becomes available.
This approach empowers nexuslua scripts to efficiently parallel-process demanding tasks while remaining responsive to user inputs or other events.

When executing the command line interface of nexuslua, the script may call `send` with a `reply_to` subtable specifying `main` as the callback agent, because the command line interface internally executed the script an agent with name `"main"`.

# See also

- [addagent](addagent.md)
- [addmessage](addmessage.md)
- [isreplicated](isreplicated.md)
- [cores](cores.md)
