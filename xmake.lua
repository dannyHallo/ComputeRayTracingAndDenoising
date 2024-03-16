set_project("voxel-lab")

set_arch("x64")
set_warnings("all")
set_languages("c++20")

-- https://xmake.io/mirror/manual/custom_toolchain.html
-- clang 17 is explicitly used here to because the compiler might not be in the PATH
toolchain("clang17")
    set_kind("standalone")
    set_bindir("D:/LLVM17/bin/") 
    set_toolset("cc", "clang") 
    set_toolset("cxx", "clang++")
toolchain_end() 
-- clang is preferred for its compatibility to fully enable the cache compiling features

-- the entire build requires clang17
set_toolchains("clang17")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

target("imgui")
    set_kind("static")
    add_files(
        "dep/imgui/imgui_demo.cpp",
        "dep/imgui/imgui_draw.cpp",
        "dep/imgui/imgui_tables.cpp",
        "dep/imgui/imgui_widgets.cpp",
        "dep/imgui/imgui.cpp"
    )
    add_includedirs(
        "dep/imgui/"
    )

target("implot")
    set_kind("static")
    add_files(
        "dep/implot/implot.cpp",
        "dep/implot/implot_demo.cpp",
        "dep/implot/implot_items.cpp"
    )
    add_includedirs(
        "dep/implot/",
        "dep/imgui/" -- implot needs imgui
    )

target("main")
    -- https://github.com/KhronosGroup/Vulkan-Loader/issues/552#issuecomment-791983003
    add_defines("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1=1")
    add_defines("DISABLE_LAYER_NV_OPTIMUS_1=1")

    -- use stl version of std::min(), std::max(), rather than the same named macros provided by windows.h
    add_defines("NOMINMAX")

    -- disable validation layers in release mode
    if is_mode("release") then
        add_defines("NVALIDATIONLAYERS")
    end

    -- RootDir.h generation
    set_configvar("PROJECT_DIR", (os.projectdir():gsub("\\", "/")))
    set_configdir("src/utils/config/")
    add_configfiles("src/utils/config/RootDir.h.in")

    -- recursivelly add all cpp files in src, to make compile units
    add_files("src/**.cpp")

    add_includedirs(
        "./",
        "src/", 
        "resources/shaders/include/",
        "dep/",
        "dep/spdlog/include/",
        "dep/imgui/",
        "dep/implot/",
        "dep/obj-loader/",
        "dep/vulkan-memory-allocator/",
        "dep/efsw/include/",
        "dep/shaderc/include",
        "dep/entt/include",
        "dep/tomlplusplus/include/"
    )

    add_deps("imgui")
    add_deps("implot")

    add_links( 
    "dep/glfw/Release/glfw3.lib", 
    "dep/shaderc/bin/shaderc_shared.lib",
    "dep/efsw/bin/efsw.lib",
    "User32", 
    "Gdi32", 
    "shell32",
    "msvcrt"
    )
