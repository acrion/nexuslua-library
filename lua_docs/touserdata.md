touserdata {#touserdata}
========

The nexuslua function [touserdata](touserdata.md) converts the given string to the Lua
type [light user data](https://www.lua.org/manual/5.4/manual.html#2.1). It accepts a single parameter containing the
string to convert and returns the value as light user data.

For an example, see [poke](poke.md).

Converting a C++ pointer to the correct format that [touserdata](touserdata.md) can parse is done by assigning that
pointer to an [`cbeam::container::xpod::type`](https://cbeam.org/doxygen/namespacecbeam_1_1container_1_1xpod.html#a69741fbc8b35de8842ecf4b76e92de58), which can be \ref nexuslua::AgentMessage::Send "sent" as part of message
parameters. Example from [acrion image framework](https://github.com/acrion/acrion-image-framework):

    explicit operator nexuslua::LuaTable() const
    {
        if (Empty())
        {
            throw std::runtime_error("Error converting image to plugin parameters: image is empty");
        }

        nexuslua::LuaTable parameters;
        parameters.data[std::string(bufferKey)]        = Buffer();
        parameters.data[std::string(widthKey)]         = Width();
        parameters.data[std::string(heightKey)]        = Height();
        parameters.data[std::string(channelsKey)]      = Channels();
        parameters.data[std::string(depthKey)]         = Depth();
        parameters.data[std::string(minBrightnessKey)] = GetMinDisplayedBrightness();
        parameters.data[std::string(maxBrightnessKey)] = GetMaxDisplayedBrightness();

        return parameters;
    }

The function `Buffer()` returns the pointer to the image buffer, which is assigned to an entry in a \ref nexuslua::LuaTable "LuaTable"
that is later sent to an nexuslua agent as follows (example from [acrionphoto](https://github.com/acrion/acrionphoto)):

    void PluginManager::RunIoPluginMessage(const nexuslua::AgentMessage*                message,
                                        const std::string&                              fileName,
                                        std::shared_ptr<acrion::imageframework::Bitmap> pWorkingImage,
                                        std::shared_ptr<acrion::imageframework::Bitmap> pReferenceImage)
    {
        bool        save;
        std::string filter;
        std::string title;

        const nexuslua::LuaTable     predefinedValues                             = PluginManager::AddImageParameters(pReferenceImage, pWorkingImage, nexuslua::LuaTable());
        nexuslua::LuaTable           parameterValues                              = predefinedValues;
        const nexuslua::sub_tables parameterDescriptionsWithDefaultValueOrUnset = message->GetDescriptionsOfUnsetParameters(predefinedValues);

        if (!areIOparams(parameterDescriptionsWithDefaultValueOrUnset, save, filter, title))
        {
            throw std::runtime_error("PluginManager::RunIoPluginMessage: '" + message->GetDisplayName() + "' is no I/O plugin message");
        }

        parameterValues.data[title] = fileName;
        parameterValues.SetReplyTo(Common::GetProductName().toStdString(), Common::GetReplyToMessageName());
        message->Send(parameterValues);
    }

The \ref nexuslua::LuaTable "LuaTable" `parameterValues` includes the pointer to the buffer and is sent to an nexuslua
agent via nexuslua::AgentMessage::Send in the last line of this method.

# See also

- [addoffset](addoffset.md)
- [peek](peek.md)
- [poke](poke.md)
