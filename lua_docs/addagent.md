addagent {#addagent}
========

The [addagent](addagent.md) function in nexuslua offers a mechanism to spawn a new operating system thread.
Once created, this thread awaits messages, executing the provided Lua code whenever a message is sent to this agent via the [send](send.md) function.

# Parameters

The function takes in two distinct parameters and an optional third:

1. **Agent Name**: A string denoting the name of the new agent.
2. **Lua Code**: A string containing the Lua code that the agent will execute. For longer scripts or for clarity, you can use the Lua multiline string syntax: `[==[` Lua code `]==]`.
3. **Message Names**: An optional list of message names that the agent shall accept. This is syntactic sugar for the use of [addmessage](addmessage.md) from within the agent code itself and does not provide the possibility to specify message meta data.

# Basic Usage

Consider this example where an agent named "numbers" is created using code from an external file, `numbers.lua`:

```lua
addagent("numbers", readfile("numbers.lua"))
```

The `numbers.lua` file might contain functions like `IsPrime`, as demonstrated in this snippet:

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

    return {isPrime=found}
end

addmessage("IsPrime")
```

With the agent now active, you can send messages to it, prompting the execution of the `IsPrime` function in an asynchronous fashion. For a hands-on demonstration of this process, please refer to the [send](send.md) documentation.

The last line `addmessage("IsPrime")` registers this message name to the agent, so it accepts messages of this name and calls the equally named function with all parameters that the sender adds to the [send](send.md) command, for example

```lua
send("numbers", "IsPrime", {number=127})
```

(See [send](send.md) for an example on how to evaluate the result of `IsPrime`.)

# Alternative way to provide message names

As syntactic sugar, `addagent` accepts an optional third parameter with a list of messages to register:

```lua
addagent("numbers", readfile("numbers.lua"), {"IsPrime"})
```

In this case, the agent itself does not need to call [addmessage](addmessage.md) as in the example above.

# Key Insights

It's essential to comprehend that `addagent` isn't merely about defining functions but setting up independent execution threads. These agents operate in parallel, responding to messages sent to them and thus enable concurrent processing in your nexuslua scripts.

When executing the command line interface of nexuslua, the provided script is executed as an agent with name `"main"`.

# See also

- [addmessage](addmessage.md)
- [send](send.md)
- [isreplicated](isreplicated.md)
- [readfile](readfile.md)

---

As before, the focus has been on clarity and breaking down the information into easily digestible chunks. The segment titled "Key Insights" is an optional addition to give users a quick conceptual overview of the function's significance. If it seems out of place or unnecessary for your documentation style, feel free to exclude it.