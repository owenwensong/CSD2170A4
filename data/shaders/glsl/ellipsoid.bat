@ECHO OFF
%VULKAN_SDK%/Bin/glslangValidator.exe -V "ellipsoid.vert" -o "ellipsoid.vert.spv"
%VULKAN_SDK%/Bin/glslangValidator.exe -V "ellipsoid.frag" -o "ellipsoid.frag.spv"
%VULKAN_SDK%/Bin/glslangValidator.exe -V "ellipsoid.tesc" -o "ellipsoid.tesc.spv"
%VULKAN_SDK%/Bin/glslangValidator.exe -V "ellipsoid.tese" -o "ellipsoid.tese.spv"
PAUSE