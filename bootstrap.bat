@echo off

@REM git submodule update --init --recursive
 
@REM git submodule add <url> <path>
@REM git rm <path>

@REM if dep/vcpkg/vcpkg.exe does not exist, bootstrap it
if not exist dep/vcpkg/vcpkg.exe (
    echo Bootstrapping vcpkg...
    dep/vcpkg/bootstrap-vcpkg.bat
) else (
    echo vcpkg already bootstrapped.
)

@REM echo Installing dependencies...
@REM start /wait /b /d "dep/vcpkg/" vcpkg.exe install 
