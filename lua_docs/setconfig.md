setconfig {#setconfig}
========

The nexuslua function [setconfig](setconfig.md) sets a table that is meant to be used as a global configuration for all
nexuslua agents. This table contains a sub table \ref nexuslua::Configuration::internal "internal" that nexuslua
relies on internally.

# Example

    local config = getconfig()
    config.demo_entry1 = "my string"
    config.demo_entry2 = 42
    config.demo_key = {}
    config.demo_key.demo_entry3 = 3.14
    printtable(config)
    setconfig(config)

## Output

    demo_entry1     my string
    demo_entry2     42
    demo_key
                    demo_entry3     3.14
    internal
                    logMessages     false
                    logReplication  false
                    luaStartNewThreadTime   0.01

# Also see

- [getconfig](getconfig.md)
- \ref nexuslua::Configuration::luaStartNewThreadTime "luaStartNewThreadTime"
- \ref nexuslua::Configuration::logMessages "logMessages"
- \ref nexuslua::Configuration::logReplication "logReplication"
- [printtable](printtable.md)
- [send](send.md)

