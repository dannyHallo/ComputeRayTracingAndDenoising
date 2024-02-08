set_project("Voxel Lab")

set_arch("x64")
set_warnings("all")
set_languages("c++20")
set_toolchains("clang")

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
    if is_mode("release") then
        add_defines("NVALIDATIONLAYERS")
    end
    -- add_defines("NDEBUG")
    set_configvar("PROJECT_DIR", (os.projectdir():gsub("\\", "/")))
    set_configdir("src/utils/config/")
    add_configfiles("src/utils/config/RootDir.h.in")
    -- recursivelly add all cpp files in src, to make compile units
    add_files("src/**.cpp")
    add_files(
        "dep/imgui/backends/imgui_impl_glfw.cpp", 
        "dep/imgui/backends/imgui_impl_vulkan.cpp"
    )
    add_includedirs(
        "./",
        "src/",
        "dep/",
        "dep/spdlog/include/",
        "dep/imgui/",
        "dep/implot/",
        "dep/obj-loader/",
        "dep/memory-allocator-hpp/",
        "shaders/generated/"
    )
    add_deps("imgui")
    add_deps("implot")

    add_links( 
    "dep/glfw/Release/glfw3.lib", 
    "User32", 
    "Gdi32", 
    "shell32",
    "msvcrt"
    )
