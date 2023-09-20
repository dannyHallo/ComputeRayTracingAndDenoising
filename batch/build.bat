@echo off
setlocal EnableDelayedExpansion

set BUILD_TYPE=debug

set GLSLC=%VULKAN_SDK%/Bin/glslc.exe

set GLSLC_PATH=%GLSLC%
set SHADERS=rtx blurPhase1 blurPhase2 blurPhase3 temporalFilter

echo Compiling shaders...
if not exist "resources/shaders/generated" mkdir "resources/shaders/generated"
for %%s in (%SHADERS%) do (
   echo Compiling resources/shaders/source/%%s.comp to resources/shaders/generated/%%s.spv
    "%GLSLC_PATH%" resources/shaders/source/%%s.comp -o resources/shaders/generated/%%s.spv
    if !errorlevel! neq 0 (
        echo Build failed with error %errorlevel%. Exiting... 
        goto :eof
    )
)

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

