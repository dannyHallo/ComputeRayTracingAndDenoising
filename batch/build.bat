@echo off
setlocal EnableDelayedExpansion

if "%1"=="debug" (
    set BUILD_TYPE=debug
) else if "%1"=="release" (
    set BUILD_TYPE=release
) else (
    echo invalid build type "%1". Exiting...
    goto :eof
)

@REM ---------------------------------------------------------------------------------------

@REM set GLSLC=%VULKAN_SDK%/Bin/glslc.exe
@REM set SHADERS=chunkFieldConstruction chunkVoxelCreation chunkModifyArg octreeInitNode octreeTagNode octreeAllocNode octreeModifyArg svoCoarseBeam svoTracing temporalFilter aTrous postProcessing 

@REM https://chromium.googlesource.com/external/github.com/google/shaderc/+/HEAD/glslc/README.asciidoc
@REM echo compiling shaders...
@REM if exist "resources/compiled-shaders" rd /s /q "resources/compiled-shaders"
@REM mkdir "resources/compiled-shaders"
@REM for %%s in (%SHADERS%) do (
@REM    echo compiling src/shaders/%%s.comp to resources/compiled-shaders/%%s.spv
@REM     "%GLSLC%" src/shaders/%%s.comp -o resources/compiled-shaders/%%s.spv --target-env=vulkan1.1
@REM     if !errorlevel! neq 0 (
@REM         echo Build failed with error %errorlevel%. Exiting... 
@REM         goto :eof
@REM     )
@REM )

@REM ---------------------------------------------------------------------------------------

echo:
echo compiling cpp ...
xmake f -p windows -a x64 -m %BUILD_TYPE%
xmake -w
if !errorlevel! neq 0 (
   echo Build failed with error !errorlevel!. Exiting... 
   goto :eof
)
echo xmake success

@REM ---------------------------------------------------------------------------------------

if %BUILD_TYPE%==release (
    echo:
    echo copying resources for release mode...
    if exist "build/windows/x64/%BUILD_TYPE%/resources" rd /s /q "build/windows/x64/%BUILD_TYPE%/resources"
    mkdir "build/windows/x64/%BUILD_TYPE%/resources"
    robocopy "resources" "build/windows/x64/%BUILD_TYPE%/resources" /E /IS /NFL /NDL /NJH /NJS /nc /ns /np
) else (
    echo:
    echo debug mode, skip resources copying for shader hot-reloading...
)

@REM ---------------------------------------------------------------------------------------

echo:
echo prepare dlls ...
robocopy "dep/efsw/bin/" "build/windows/x64/%BUILD_TYPE%/" "efsw.dll" /NFL /NDL /NJH /NJS /nc /ns /np
robocopy "dep/shaderc/bin/" "build/windows/x64/%BUILD_TYPE%/" "shaderc_shared.dll" /NFL /NDL /NJH /NJS /nc /ns /np

@REM ---------------------------------------------------------------------------------------

echo:
echo run app ...
echo ---------------------------------------------------------------------------------------
echo:

@REM run the application
@REM /b means to stay in the command line below, 
@REM /wait blocks the terminal to wait for the application to exit
start /wait /b /d "build/windows/x64/%BUILD_TYPE%" main.exe
