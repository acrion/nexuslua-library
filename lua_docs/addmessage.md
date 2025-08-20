addmessage {#addmessage}
==========

Within nexuslua, the [addmessage](addmessage.md) function plays a pivotal role in defining the communication interface for an agent.
It facilitates the registration of new messages that the agent can receive and process, either from other nexuslua scripts or C++ applications.

Essentially, when you register a message using `addmessage`, you're setting up an entry point—a way for other parts of your system to make asynchronous function calls to this agent.

The function accepts the message name to be registered as first parameter.
Optionally, a second parameter may be provided, see below section **Advanced Usage with Metadata**.

# Basic Usage

A prime example is the `IsPrime` function. It determines if an input number is prime and sends back a corresponding result:

```lua
function IsPrime(parameters)
    local number = tonumber(parameters.number)
    q = math.sqrt(number)
    found = true
    for k=3,q,2 do
        d = number/k
        di = math.floor(d)
        if d == di then
            found = false
            break
        end
    end

    return {isPrime=found}  -- This result is sent back to the agent designated in the reply_to table of the sent message.
end

addmessage("IsPrime")
```

The function expects a single integer number as a parameter, and the message can be dispatched using the `send` function:

```lua
send("numbers", "IsPrime", {number=127, reply_to={agent="main", message="CountPrime"}})
```

Here, besides specifying the number for the prime check, the example dictates where the `{isPrime=found}` result should be relayed.

# Advanced Usage with Metadata

For applications like the [acrion image tools](https://github.com/acrion/image-tools) that employ a GUI, messages often require additional metadata, such as a display name, description, or icon.
Moreover, even though nexuslua natively recognizes Lua types, these messages can further describe the expected parameters, including their type, for better interface clarity:

```lua
addmessage("CallSubtractLeftRightLua", {
    displayname="left-right (Lua)",
    description="Subtract working image from reference using Lua (click twice to undo)",
    icon="",
    parameters={
        workingImage   = { type = "void*"},
        referenceImage = { type = "void*"},
        width          = { type = "long long"},
        height         = { type = "long long"},
        channels       = { type = "long long"},
        depth          = { type = "long long"}
}})
```

However, it's crucial to understand that this type information and metadata have no direct impact on the nexuslua's functioning.
They primarily serve the GUI to render and interpret the parameters appropriately.

# Key Insights

With `addmessage`, an agent outlines how it should respond to a specific message.
It's not merely a function for adding functionalities but a method for defining communication protocols between agents.
When an agent receives a message it has registered via `addmessage`, it knows precisely what action to undertake.
The addition of metadata, as demonstrated in the "acrion image" example, illustrates that messages can also serve to describe agents in a user interface, enhancing nexuslua's interoperability and user-friendliness.

When executing the command line interface of nexuslua, the script may call `addmessage`, because it is internally executed as an agent with name `"main"`.

# See also

- [addagent](addagent.md)
- [send](send.md)
- [replicated](isreplicated)
