# fetches all submodules for this project
git submodule update --init --recursive

# bootstrap vcpkg if not already bootstrapped
if [ ! -f dep/vcpkg/vcpkg ]; then
    echo Bootstrapping vcpkg...
    cd dep/vcpkg
    sh bootstrap-vcpkg.sh
else
    echo vcpkg already bootstrapped.
fi
