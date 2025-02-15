set_project("DragonTweak")
add_rules("mode.debug", "mode.release")
set_languages("c++latest", "clatest")
set_optimize("faster")

target("DragonTweak")
  set_kind("shared")
  add_files("./src/**.cpp", "./external/safetyhook/safetyhook.cpp", "./external/safetyhook/zydis.c")
  add_syslinks("user32")
  add_includedirs("./external/spdlog/include", "./external/inipp", "./external/safetyhook")
  set_extension(".asi")

  set_toolchains("clang")
  add_cxflags("-static")