@echo off

set GLSLC=%VULKAN_SDK%/Bin/glslc
@REM set GLSLANG=%VULKAN_SDK%/Bin/glslangValidator.exe

echo Compiling shaders...

%GLSLC% resources/shaders/source/post-process-shader.vert -o resources/shaders/generated/post-process-vert.spv
%GLSLC% resources/shaders/source/post-process-shader.frag -o resources/shaders/generated/post-process-frag.spv
%GLSLC% resources/shaders/source/rtx.comp -o resources/shaders/generated/rtx.spv
%GLSLC% resources/shaders/source/blurPhase1.comp -o resources/shaders/generated/blurPhase1.spv
%GLSLC% resources/shaders/source/blurPhase2.comp -o resources/shaders/generated/blurPhase2.spv
%GLSLC% resources/shaders/source/blurPhase3.comp -o resources/shaders/generated/blurPhase3.spv
%GLSLC% resources/shaders/source/temporalFilter.comp -o resources/shaders/generated/temporalFilter.spv

echo done

mkdir build
cd build
cmake ..
cd ..

MSBuild build/vulkan.sln -p:Configuration=Debug

start /d "./build/Debug" vulkan.exe