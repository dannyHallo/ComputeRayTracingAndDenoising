@echo off

set PROJECT_NAME=pathtracer
set BUILD_TYPE=Debug

set GLSLC=%VULKAN_SDK%/Bin/glslc.exe

echo:
echo *********************** compile shaders ***********************
echo:

%GLSLC% resources/shaders/source/post-process-shader.vert -o resources/shaders/generated/post-process-vert.spv
%GLSLC% resources/shaders/source/post-process-shader.frag -o resources/shaders/generated/post-process-frag.spv
%GLSLC% resources/shaders/source/rtx.comp -o resources/shaders/generated/rtx.spv
%GLSLC% resources/shaders/source/blurPhase1.comp -o resources/shaders/generated/blurPhase1.spv
%GLSLC% resources/shaders/source/blurPhase2.comp -o resources/shaders/generated/blurPhase2.spv
%GLSLC% resources/shaders/source/blurPhase3.comp -o resources/shaders/generated/blurPhase3.spv
%GLSLC% resources/shaders/source/temporalFilter.comp -o resources/shaders/generated/temporalFilter.spv

echo:
@echo *************************** cmake ****************************
echo:

mkdir build
cd build
cmake .. -D PROJ_NAME=%PROJECT_NAME%
cd ..

echo:
@echo ************************** msbuild ***************************
echo:

MSBuild build/%PROJECT_NAME%.sln -p:Configuration=%BUILD_TYPE%

echo:
@echo *********************** copy resources ***********************
echo:

mkdir "build/%BUILD_TYPE%/resources"
xcopy "resources" "build/%BUILD_TYPE%/resources" /s /i /y

echo:
@echo **************************** run *****************************
echo:

start /d "./build/%BUILD_TYPE%" %PROJECT_NAME%.exe