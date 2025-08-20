getconfig {#getconfig}
========

The nexuslua function [getconfig](getconfig.md) returns a table that is meant to be used as global configuration for all
running agents.
The sub table \ref nexuslua::Configuration::internal "internal" contains entries that are used by nexuslua internally.
Currently there are three entries:

- \ref nexuslua::Configuration::luaStartNewThreadTime "luaStartNewThreadTime" contains the default time after an agent
  will create a new operating system thread to distribute its workload, until at most as many threads are running for
  this agent as specified in the message that is currently processed.
- \ref nexuslua::Configuration::logMessages "logMessages" after setting this to true, all nexuslua messages for newly
  created agents will be logged to "nexuslua.log" in the [user folder](https://cbeam.org/doxygen/namespacecbeam_1_1filesystem.html#ae598d93475d7f8675bb85d7542cf90ab)
- \ref nexuslua::Configuration::logReplication "logReplication" after setting this to true, each time an agent is
  replicated (creating a new thread) a corresponding log entry is created in file "nexuslua.log" in the [user folder](https://cbeam.org/doxygen/namespacecbeam_1_1filesystem.html#ae598d93475d7f8675bb85d7542cf90ab)

See [setconfig](setconfig.md) for an example.

# Also see

- [send](send.md)
- \ref nexuslua::Configuration::luaStartNewThreadTime "luaStartNewThreadTime"
- \ref nexuslua::Configuration::logMessages "logMessages"
- \ref nexuslua::Configuration::logReplication "logReplication"

