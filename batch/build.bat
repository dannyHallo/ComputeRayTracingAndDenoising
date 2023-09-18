@echo off

set BUILD_TYPE=debug

set GLSLC=%VULKAN_SDK%/Bin/glslc.exe

echo:
echo compiling shaders 
%GLSLC% resources/shaders/source/post-process-shader.vert -o resources/shaders/generated/post-process-vert.spv
%GLSLC% resources/shaders/source/post-process-shader.frag -o resources/shaders/generated/post-process-frag.spv
%GLSLC% resources/shaders/source/rtx.comp -o resources/shaders/generated/rtx.spv
%GLSLC% resources/shaders/source/blurPhase1.comp -o resources/shaders/generated/blurPhase1.spv
%GLSLC% resources/shaders/source/blurPhase2.comp -o resources/shaders/generated/blurPhase2.spv
%GLSLC% resources/shaders/source/blurPhase3.comp -o resources/shaders/generated/blurPhase3.spv
%GLSLC% resources/shaders/source/temporalFilter.comp -o resources/shaders/generated/temporalFilter.spv
echo done

echo:
echo xmake
xmake f -p windows -a x64 -m %BUILD_TYPE%
xmake -w
echo done

if %errorlevel% neq 0 (
   echo Build failed with error %errorlevel%. Exiting... 
   goto :eof
)

echo:
@echo copy resources
mkdir "build/windows/x64/%BUILD_TYPE%/resources"
xcopy "resources" "build/windows/x64/%BUILD_TYPE%/resources" /s /i /y

echo:
echo run

@REM run the application
@REM /b means to stay in the command line below, 
@REM /wait blocks the terminal to wait for the application to exit
start /wait /b /d "build/windows/x64/%BUILD_TYPE%" main.exe

set errorlevel=%ERRORLEVEL%

