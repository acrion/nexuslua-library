poke                         {#poke}
========

The nexuslua function `poke` writes an integer to a memory address. It accepts 3 parameters:

- the memory address as [light user data](https://www.lua.org/manual/5.4/manual.html#2.1)
- the value to write to the memory address
- the number of bytes to write (1,2,4 or 8; on some systems, 16 is supported)

# Example

This code from agent [acrion image_tools](https://github.com/acrion/image-tools) calls the poke function to write the resulting pixel value to its memory address.

    function CallSubtractLeftRightLua(parameters)
        local size = parameters.width * parameters.height * parameters.channels
        local depth = parameters.depth
        local workingImage = touserdata(parameters.workingImage)
        local referenceImage = touserdata(parameters.referenceImage)
        for i=0,size do
            local addressRight = addoffset(workingImage, i, depth)
            local addressLeft  = addoffset(referenceImage, i, depth)
            local val = peek(addressLeft, depth) - peek(addressRight, depth)
            poke(addressRight, val, depth)
        end
        
        return {}
    end

The pixel value is the mathematical difference between the pixel values in two images.
This function is directly used as an nexuslua message than can be sent via [send](send.md).
To allow this, it needs to be registered first, see the second example in [addmessage](addmessage.md).

# See also

- [addoffset](addoffset.md)
- [peek](peek.md)
- [touserdata](touserdata.md)
