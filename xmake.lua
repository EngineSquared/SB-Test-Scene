add_requires("entt", "vulkan-headers", "vulkansdk", "vulkan-hpp", "glfw", "glm", "gtest", "raylib")
add_requires("raylib")

set_project("SB-Test-Scene")
set_languages("c++20")

includes("lib/EngineSquared/xmake.lua")

target("SB-Test-Scene")
    set_kind("binary")

    add_files("src/**.cpp")

    add_deps("EngineSquared")

    add_includedirs("src/")

    add_packages("entt", "vulkansdk", "glfw", "glm", "raylib")


if is_mode("debug") then
    add_defines("DEBUG")
    set_symbols("debug")
    set_optimize("none")
end

if is_mode("release") then
    set_optimize("fastest")
end