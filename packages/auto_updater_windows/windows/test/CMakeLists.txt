cmake_minimum_required(VERSION 3.14)
project(AutoUpdaterWindowsTests LANGUAGES CXX RC)
set(FLUTTER_ENGINE_DIR "" CACHE PATH "Flutter bin/cache/artifacts/engine/windows-x64")
if(NOT EXISTS "${FLUTTER_ENGINE_DIR}/flutter_windows.dll.lib")
  message(FATAL_ERROR "Pass -DFLUTTER_ENGINE_DIR=<Flutter SDK>/bin/cache/artifacts/engine/windows-x64")
endif()
set(CMAKE_CXX_STANDARD 17)
add_library(flutter INTERFACE)
target_include_directories(flutter INTERFACE "${FLUTTER_ENGINE_DIR}")
target_link_libraries(flutter INTERFACE "${FLUTTER_ENGINE_DIR}/flutter_windows.dll.lib")
add_library(flutter_wrapper_plugin STATIC
  "${FLUTTER_ENGINE_DIR}/cpp_client_wrapper/core_implementations.cc"
  "${FLUTTER_ENGINE_DIR}/cpp_client_wrapper/standard_codec.cc"
  "${FLUTTER_ENGINE_DIR}/cpp_client_wrapper/plugin_registrar.cc")
target_include_directories(flutter_wrapper_plugin PUBLIC
  "${FLUTTER_ENGINE_DIR}/cpp_client_wrapper/include")
target_link_libraries(flutter_wrapper_plugin PUBLIC flutter)
function(apply_standard_settings target)
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_compile_options(${target} PRIVATE /W4 /WX /wd4100)
endfunction()
set(FLUTTER_LIBRARY "${FLUTTER_ENGINE_DIR}/flutter_windows.dll")
set(include_auto_updater_windows_tests TRUE)
enable_testing()
add_subdirectory(.. plugin)
foreach(variant high low)
  if(variant STREQUAL "high")
    set(FIXTURE_FILE_VERSION "9,9,9,999")
    set(FIXTURE_FILE_VERSION_STRING "9.9.9.999")
    set(FIXTURE_PRODUCT_VERSION "9.9.9+999")
  else()
    set(FIXTURE_FILE_VERSION "0,0,1,1")
    set(FIXTURE_FILE_VERSION_STRING "0.0.1.1")
    set(FIXTURE_PRODUCT_VERSION "0.0.1+1")
  endif()
  configure_file(identity_fixture.rc.in "${variant}.rc" @ONLY)
  add_executable(winsparkle_identity_${variant}
    winsparkle_identity_fixture.cpp "${CMAKE_CURRENT_BINARY_DIR}/${variant}.rc")
  target_link_libraries(winsparkle_identity_${variant} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../WinSparkle-0.8.1/x64/Release/WinSparkle.lib"
    Advapi32)
  add_custom_command(TARGET winsparkle_identity_${variant} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/../WinSparkle-0.8.1/x64/Release/WinSparkle.dll"
    $<TARGET_FILE_DIR:winsparkle_identity_${variant}>)
endforeach()
