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

set PROJECT_NAME=voxel-lab
set PROJECT_GENERATOR=Ninja

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

if not exist build mkdir build

@REM ---------------------------------------------------------------------------------------


echo:
echo configuring for the generator %PROJECT_GENERATOR%...
cmake -S . -B build/ ^
    -G %PROJECT_GENERATOR% ^
    -D CMAKE_PROJECT_NAME=%PROJECT_NAME% ^
    -D CMAKE_TOOLCHAIN_FILE="../dep/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -D VCPKG_TARGET_TRIPLET="x64-windows" ^
    -D CMAKE_CXX_COMPILER="D:/LLVM17/bin/clang.exe" ^
    -D CMAKE_EXPORT_COMPILE_COMMANDS=1 ^
    -D CMAKE_CXX_COMPILER_LAUNCHER=ccache ^
    -D CMAKE_BUILD_TYPE=%BUILD_TYPE%

if !errorlevel! neq 0 (
   echo "CMake config failed with error !errorlevel!. Exiting... "
   goto :eof
)

echo:
echo generating project using %PROJECT_GENERATOR%...
echo ---------------------------------------------------------------------------------------
echo:

cmake --build build/

set PROJECT_EXECUTABLE_PATH="build/apps/"

@REM ---------------------------------------------------------------------------------------

@REM if %BUILD_TYPE%==release (
@REM     echo:
@REM     echo copying resources for release mode...
@REM     if exist "build/windows/x64/%BUILD_TYPE%/resources" rd /s /q "build/windows/x64/%BUILD_TYPE%/resources"
@REM     mkdir "build/windows/x64/%BUILD_TYPE%/resources"
@REM     robocopy "resources" "build/windows/x64/%BUILD_TYPE%/resources" /E /IS /NFL /NDL /NJH /NJS /nc /ns /np
@REM ) else (
@REM     echo:
@REM     echo debug mode, skip resources copying for shader hot-reloading...
@REM )

@REM @REM ---------------------------------------------------------------------------------------

echo:
echo run app ...
echo ---------------------------------------------------------------------------------------
echo:

@REM run the application
@REM /wait blocks the terminal to wait for the application to exit
@REM /b means to stay in the command line below, 
@REM /d xxx specifies the startup directory
start /wait /b /d %PROJECT_EXECUTABLE_PATH% %PROJECT_NAME%.exe
