cl /MD /I C:\VulkanSDK\glfw-3.3.9.bin.WIN64\include /I .\headers\ /I C:\VulkanSDK\1.3.275.0\Include /std:c++20 /EHsc %1.cpp gdi32.lib opengl32.lib kernel32.lib user32.lib shell32.lib glfw3.lib vulkan-1.lib /link /LIBPATH:C:\VulkanSDK\glfw-3.3.9.bin.WIN64\lib-vc2022 /LIBPATH:C:\VulkanSDK\1.3.275.0\Lib
cd shaders
glslc CarShader.frag -o CarFrag.spv
glslc CarShader.vert -o CarVert.spv
glslc SkyBoxShader.frag -o SkyBoxFrag.spv
cd..