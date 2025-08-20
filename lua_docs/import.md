import {#import}
======

The `import` function in nexuslua offers the ability to bridge C++ shared libraries with nexuslua's scripting interface.
By using `import`, one can utilize functions from shared libraries (*.dll, *.so, *.dylib) directly within nexuslua
scripts.

# Parameters

1. **Shared Library Name**: This is the name of the shared library without the file extension.
2. **Function Name**: The name of the function you wish to make available in nexuslua.
3. **Signature**: Describes the return type (which could be `void` or any of the below types) and a comma separated
   list (x,y,...) of types that the function accepts. These types are:
    - **`table`**: A Lua table corresponding to C++ type `nexuslua::LuaTable`. This type primarily consists of key-value
      pairs represented by [`cbeam::container::nested_map::table_of_values`](https://cbeam.org/doxygen/structcbeam_1_1container_1_1nested__map.html#a3e1c5f1d8eb1ab17456dd148d7215661)
      and sub-tables defined by
      [`cbeam::container::nested_map::nested_tables`](https://cbeam.org/doxygen/structcbeam_1_1container_1_1nested__map.html#a2d70cf56502d98efa974d5b711256252).
      The `table_of_values` contains data in the form of the central
      [`cbeam::container::xpod::type`](https://cbeam.org/doxygen/namespacecbeam_1_1container_1_1xpod.html#a69741fbc8b35de8842ecf4b76e92de58) std::variant. This std::variant
      can hold various data types including `long long`, `double`, `bool`, `memory::pointer`, and `std::string`.
    - **`void*`**: Can be converted to Lua userdata type via the function [touserdata](touserdata.md)
    - **`long long`**: Corresponds to Lua integer type.
    - **`double`**: Corresponds to Lua number type.
    - **`bool`**: Corresponds to Lua ... type.
    - **`const char*`**: Corresponds to Lua ... type

The total number of arguments allowed is 3 (configurable
via [max_arguments](https://github.com/acrion/nexuslua-library/blob/maain/src/lua_code_generator_find_signature.lua)) and the
sequence must follow the list above.
Given that the type @ref nexuslua::LuaTable "LuaTable" can encapsulate an indeterminate number of values, these constraints
primarily cater to shared libraries not linking the [https://github.com/acrion/Cbeam](Cbeam) library, a prerequisite for unlocking the type
@ref nexuslua::LuaTable "LuaTable".
Note that a table is automatically serialized to make use of the advantages of the extern "C" interface.
The order and limits can be tweaked within the code generator lua_code_generator.lua.

# Note on Memory Management

During interactions between Lua and C++, it's crucial to understand the intricacies of memory management.
While shared libraries can allocate their memory as they typically would, there are specific requirements for memory
that is exchanged between the shared library and the Lua script.

Any memory area passed back to the caller – be it the Lua script which imported the shared library or a C++ application
running this Lua script as an nexuslua agent – must be mandatorily allocated using nexuslua::Memory.
This ensures proper memory management and safeguards against premature deallocations, especially when their sole
reference exists in pure Lua context.

For developers creating shared libraries, it's imperative to consider memory management when their functions are
imported and utilized within nexuslua.
Utilizing nexuslua::Memory ensures proper and seamless memory handling, including automatic deallocation, even if the
memory is only referenced from within Lua code.

# Practical Example

A real-world implementation can be observed in the [acrion image tools](https://github.com/acrion/image-tools) agent.
Here, the `import` function is used to integrate the `OpenImageFile` function from the shared library (
acrion_image_tools.dll / acrion_image_tools.dylib / libacrion_image_tools.so)

```lua
function CallOpenImageFile(parameters)
    import("acrion_image_tools", "OpenImageFile", "table(const char*)")
    return OpenImageFile(parameters.path)
end
```

The associated C++ function looks as follows:

```cpp
extern "C" ACRION_IMAGE_TOOLS_EXPORT acrion::image::SerializedBitmapContainer OpenImageFile(const char* fileName)
{
    // implementation details, using one of the following two constructors to allocate memory:
    stable_reference_buffer(const std::size_t size, const size_t size_of_type = 1); // create a managed memory block of the given size
    explicit stable_reference_buffer(const void* address);                          // create a managed memory block from preallocated address
    
    auto buffer = cbeam::serialization::serialize(image);
    return buffer.get();
}
```

Cbeam offers [template based serialization functions](https://cbeam.org/doxygen/namespacecbeam_1_1serialization.html).
In this example, the type of
`image` inherits from [
`cbeam::container::nested_map`](https://cbeam.org/doxygen/structcbeam_1_1container_1_1nested__map.html),
which is one of the various types that enables serialization. Of course Cbeam allows to declare your own serializable types,
see [Cbeam’s serialization namespace](https://github.com/acrion/Cbeam/tree/main/include/cbeam/serialization) for example on how to
add serialization functionality to existing types (you don't need to change your actual types to do this).

# Related References

- **[`cbeam::container::stable_reference_buffer`](https://cbeam.org/doxygen/classcbeam_1_1container_1_1stable__reference__buffer.html)**: Manages memory allocation and deallocation for shared libraries loaded by a Lua agent via
  `import`.
- **[`cbeam::memory::pointer`](https://cbeam.org/doxygen/classcbeam_1_1memory_1_1pointer.html)**: A class within nexuslua which facilitates the sharing of memory addresses in messages between
  nexuslua and C++.

To fully harness the power of `import` and ensure smooth interoperability between Lua scripts and C++ functions,
understanding memory management and the role of [`cbeam::container::xpod::type`](https://cbeam.org/doxygen/namespacecbeam_1_1container_1_1xpod.html#a69741fbc8b35de8842ecf4b76e92de58) as a std::variant is essential.
