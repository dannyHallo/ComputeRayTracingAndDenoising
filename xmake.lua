set_project("vulkan test")

set_arch("x64")
set_warnings("all")
set_languages("c++17")
-- set_toolchains("msvc")
set_toolchains("clang")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

target("main")
    set_default(true)
    set_kind("binary")
    add_files("src/**.cpp")
    add_files(
        "dep/imgui/backends/imgui_impl_glfw.cpp", 
        "dep/imgui/backends/imgui_impl_vulkan.cpp",
        "dep/imgui/imgui_demo.cpp",
        "dep/imgui/imgui_draw.cpp",
        "dep/imgui/imgui_tables.cpp",
        "dep/imgui/imgui_widgets.cpp",
        "dep/imgui/imgui.cpp"
    )
    add_includedirs(
        "src",
        "dep",
        "dep/spdlog/include",
        "dep/glfw",
        "dep/glm",
        "dep/imgui",
        "dep/volk",
        "dep/stb",
        "dep/obj-loader",
        "dep/memory-allocator-hpp"
    )
    add_linkdirs("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64")
    add_linkdirs("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.36.32532/lib/x64")
    add_links(
    "dep/glfw/Release/glfw3.lib", 
    "User32", 
    "Gdi32", 
    "shell32",
    "msvcrt"
    )
