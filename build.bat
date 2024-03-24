@echo off
setlocal EnableDelayedExpansion

set PROJECT_NAME=voxel-lab
set PROJECT_GENERATOR=Ninja
set PROJECT_EXECUTABLE_PATH=build/apps/
set PROJECT_TARGET_TRIPLET=x64-windows
set PROJECT_CLANG_PATH=D:/LLVM17/bin/clang.exe

set BUILD_TYPE=release
set SKIP_CONFIG=OFF

FOR %%a IN (%*) DO (
    if [%%a] == [--debug] set BUILD_TYPE=debug
    if [%%a] == [--skip-config] set SKIP_CONFIG=ON
)

if %SKIP_CONFIG%==OFF (
cmake -S . -B build/ ^
    -G %PROJECT_GENERATOR% ^
    -D CMAKE_PROJECT_NAME=%PROJECT_NAME% ^
    -D CMAKE_TOOLCHAIN_FILE="../dep/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -D VCPKG_TARGET_TRIPLET=%PROJECT_TARGET_TRIPLET% ^
    -D CMAKE_CXX_COMPILER=%PROJECT_CLANG_PATH% ^
    -D CMAKE_EXPORT_COMPILE_COMMANDS=1 ^
    -D CMAKE_CXX_COMPILER_LAUNCHER=ccache ^
    -D CMAKE_BUILD_TYPE=%BUILD_TYPE%

if !errorlevel! neq 0 (
   echo cmake config failed
   goto :eof
)
)

if not exist build mkdir build
cmake --build build/
if !errorlevel! neq 0 (
   echo build failed
   goto :eof
)

if %BUILD_TYPE%==release (
    echo:
    echo copy resources folder for release mode...
    set RESOURCES_PATH=%PROJECT_EXECUTABLE_PATH%resources
    echo "!RESOURCES_PATH!"
    if exist "!RESOURCES_PATH!" rd /s /q "!RESOURCES_PATH!"
    mkdir "!RESOURCES_PATH!"
    robocopy "resources/" "!RESOURCES_PATH!" /E /IS /NFL /NDL /NJH /NJS /nc /ns /np
)

@REM run the application
@REM /wait blocks the terminal to wait for the application to exit
@REM /b means to stay in the command line below, 
@REM /d xxx specifies the startup directory
start /wait /b /d "%PROJECT_EXECUTABLE_PATH%" %PROJECT_NAME%.exe
