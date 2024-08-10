# exit on error
set -e

BUILD_TYPE="release"
WITH_PORTABLE_RESOURCES="OFF"

for arg in "$@"; do
    if [ "$arg" == "--debug" ]; then
        BUILD_TYPE="debug"
    fi
done

BINARY_DIR="build/$BUILD_TYPE/"
PROJECT_EXECUTABLE_PATH="${BINARY_DIR}apps/"

cmake --preset "$BUILD_TYPE" \
    -D CMAKE_TOOLCHAIN_FILE="../../dep/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -D VCPKG_MANIFEST_INSTALL=ON \
    -D WITH_PORTABLE_RESOURCES="$WITH_PORTABLE_RESOURCES"

if [ $? -ne 0 ]; then
    echo "cmake config failed"
    exit 1
fi

mkdir -p "$BINARY_DIR"
cmake --build "$BINARY_DIR"

if [ $? -ne 0 ]; then
    echo "build failed"
    exit 1
fi

# This logic is moved to CMakeLists.txt for cross-platform compatibility
# echo copy compile_commands.json to .vscode folder
# cp "$BINARY_DIR/compile_commands.json" ".vscode/"

if [ "$WITH_PORTABLE_RESOURCES" == "ON" ]; then
    echo
    echo "copy resources folder for portable build"
    RESOURCES_PATH="${PROJECT_EXECUTABLE_PATH}resources/"
    rm -rf "$RESOURCES_PATH"
    mkdir -p "$RESOURCES_PATH"
    cp -r "resources/"* "$RESOURCES_PATH"
fi

# Run the application
# The equivalent of start /wait /b is to just run the executable in the current shell
./$PROJECT_EXECUTABLE_PATH/run
