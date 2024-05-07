@echo off
setlocal EnableDelayedExpansion

set BUILD_TYPE=release
set WITH_PORTABLE_RESOURCES=OFF

FOR %%a IN (%*) DO (
    if [%%a] == [--debug] set BUILD_TYPE=debug
)

set BINARY_DIR=build/%BUILD_TYPE%/
set PROJECT_EXECUTABLE_PATH=%BINARY_DIR%apps/
 
cmake --preset %BUILD_TYPE% ^
    -D CMAKE_TOOLCHAIN_FILE="../../dep/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -D VCPKG_MANIFEST_INSTALL=ON ^
    -D WITH_PORTABLE_RESOURCES=%WITH_PORTABLE_RESOURCES%

if !errorlevel! neq 0 (
   echo cmake config failed
   goto :eof
)

if not exist %BINARY_DIR% mkdir %BINARY_DIR%
cmake --build %BINARY_DIR%

if !errorlevel! neq 0 (
   echo build failed
   goto :eof
)

@REM this logic is moved to CMakeLists.txt for cross-platform compatibility
@REM echo copy compile_commands.json to .vscode folder
@REM robocopy %BINARY_DIR% .vscode/ compile_commands.json /NFL /NDL /NJH /NJS /nc /ns /np

if %WITH_PORTABLE_RESOURCES%==ON (
    echo:
    echo copy resources folder for portable build
    set RESOURCES_PATH=%PROJECT_EXECUTABLE_PATH%resources/
    if exist "!RESOURCES_PATH!" rd /s /q "!RESOURCES_PATH!"
    mkdir "!RESOURCES_PATH!"
    robocopy "resources/" "!RESOURCES_PATH!" /E /IS /NFL /NDL /NJH /NJS /nc /ns /np
)

@REM run the application
@REM /wait blocks the terminal to wait for the application to exit
@REM /b means to stay in the command line below, 
@REM /d xxx specifies the startup directory
start /wait /b /d "%PROJECT_EXECUTABLE_PATH%" run.exe
