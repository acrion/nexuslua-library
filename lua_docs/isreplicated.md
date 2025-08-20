isreplicated                         {#isreplicated}
========

The nexuslua function [isreplicated](isreplicated.md) is pivotal for determining the instantiation status of a given agent.
It assesses if the currently executing agent instance is the primary (first) instance or if it's an auxiliary instance that was automatically spun up to handle increased workload.
Recognizing the distinction is crucial, particularly for actions that should be performed just once, like invoking [addmessage](addmessage.md) or dispatching initial messages.

# Example

```lua
    local nRequests = 0
    local nCheckedPrimes = 0
    local count = 0

    function CountPrime(parameters)
        nCheckedPrimes = nCheckedPrimes + 1

        if parameters.isPrime then
            count = count + 1
        end
        
        if (nCheckedPrimes == nRequests) then
            print("Checked ", nCheckedPrimes, " numbers, found ", count, " primes.")
        end
    end

    function RequestPrimes(parameters)
        print("Analyzing prime numbers between ", parameters.n1, " and ", parameters.n2)

        for i=parameters.n1,parameters.n2,2 do
            send("numbers", "IsPrime", {number=i, threads=16, reply_to={agent="main", message="CountPrime"}})
            nRequests = nRequests + 1
        end
    end

    if not isreplicated() then
        local numbersCode=[==[
            function IsPrime(parameters)
                local number = tonumber(parameters.number)
                q=math.sqrt(number)
                found=true
                for k=3,q,2 do
                    d = number/k
                    di = math.floor(d)
                    if d==di then
                        found=false
                        break
                    end
                end

                return {isPrime=found}
            end

            addmessage("IsPrime")
        ]==]

        addagent("numbers", numbersCode) -- an alternative approach would be to place the above code into numbers.lua and invoke addagent("numbers", readfile("numbers.lua"))
        addmessage("CountPrime")
        addmessage("RequestPrimes")

        send("main", "RequestPrimes", {n1=100000000001,
                                       n2=100000100001})
    end
```

# Key Insights

The `isreplicated` function is intended to be used as a gatekeeper, ensuring certain instructions are executed solely by the primary agent instance, and not by any replicated instances.
This distinction avoids redundancy, maintains efficiency, and ensures that specific operations, especially those related to initialization or setup, are executed only once.

# See also

- [addagent](addagent.md)
- [ddmessage](addmessage.md)
- [send](send.md)
