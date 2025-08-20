install {#install}
======

The nexuslua function [install](install.md) installs an nexuslua plugin from a zip file.
It accepts 1 parameter, the path to the plugin zip file.
It returns 0 on success, otherwise an error code corresponding to @ref ::nexuslua::PluginInstallResult "nexuslua::
PluginInstallResult"

A plugin is an [nexuslua agent](addagent.md) with meta data. It consists of at least two files:

- `main.lua` which will be executed as an [nexuslua agent](addagent.md)
- `nexuslua_plugin.toml` containing the following meta data, separated by semicolons:
    1. display name
    2. version
    3. true, if the plugin is freeware, otherwise false
    4. URL to documentation
    5. URL to download latest version
    6. URL to license
    7. optionally, if (3) has the value false, a URL where one can purchase a license

  For an example see
  the
  corresponding [template file](https://github.com/acrion/image-tools/blob/main/acrion_image-tools/nexuslua_plugin.toml.template)
  of the nexuslua image tools plugin.

TODO

# Example

  ```bash
  nexuslua -e 'install("plugin.zip")'
  ```

# See also

- @ref ::nexuslua::agents::InstallPlugin "nexuslua::agents::InstallPlugin"

