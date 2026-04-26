add_rules("mode.debug", "mode.release")
add_requires("openssl")
add_requires("cpp-httplib")
add_requires("nlohmann_json")

local is_termux = is_host("linux") and os.getenv("TERMUX_VERSION")

if is_termux then
    set_toolchains("clang")
    add_cxxflags("-isystem /data/data/com.termux/files/usr/include/c++/v1", {force = true})
end

target("client")
    set_kind("binary")
    add_files("src/Client/*.cpp")
    add_files("src/Common/*.cpp")
    add_includedirs("src/Common", "src/Client")
    add_packages("cpp-httplib", "openssl", "nlohmann_json")
    set_languages("c++17")

target("server")
    set_kind("binary")
    add_files("src/Server/*.cpp")
    add_files("src/Common/*.cpp")
    add_includedirs("src/Common", "src/Server")
    add_packages("cpp-httplib", "openssl", "nlohmann_json")
    set_languages("c++17")
