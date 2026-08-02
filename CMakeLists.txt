#
# MIT License
#
# Original work Copyright (c) 2020 Joel Winarske
# Modified work Copyright     2021 Hannes Winkler
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.10.0)

# configure options
option(BUILD_TEXT_INPUT_PLUGIN "Include the text input plugin in the finished binary. Enables text input (to flutter text fields, for example) via attached keyboards." ON)
option(BUILD_RAW_KEYBOARD_PLUGIN "Include the raw keyboard plugin in the finished binary. Enables raw keycode listening in flutter via the flutter RawKeyboard interface." ON)
option(BUILD_TEST_PLUGIN "Include the test plugin in the finished binary. Allows testing platform channel communication." OFF)

option(BUILD_GSTREAMER_VIDEO_PLAYER_PLUGIN "Include the gstreamer based video plugins in the finished binary. Allows for more stable, hardware accelerated video playback in flutter using gstreamer." ON)
option(TRY_BUILD_GSTREAMER_VIDEO_PLAYER_PLUGIN "Don't throw an error if the gstreamer libs aren't found, instead just don't build the gstreamer video player plugin in that case." ON)

option(BUILD_GSTREAMER_AUDIO_PLAYER_PLUGIN "Include the gstreamer based audio plugins in the finished binary." ON)
option(TRY_BUILD_GSTREAMER_AUDIO_PLAYER_PLUGIN "Don't throw an error if the gstreamer libs aren't found, instead just don't build gstreamer audio plugin." ON)

option(BUILD_SENTRY_PLUGIN "Include the sentry plugin in the finished binary. Allows for crash reporting to sentry.io." OFF)

set(BUILD_FLUTTER_LINUX_GTK_SHIM ON)
option(BUILD_OPENAUTOFLUTTER_PLUGIN "Build OpenAutoFlutter plugin from source." OFF)
option(BUILD_GTK_SHIM_TEST_PLUGIN "Build minimal GTK shim test plugin (gtk_shim_test)." OFF)
set(OPENAUTOFLUTTER_DIR "${CMAKE_SOURCE_DIR}/../OpenAutoFlutter" CACHE PATH "Path to the OpenAutoFlutter repo")

option(BUILD_CHARSET_CONVERTER_PLUGIN "Include the charset converter plugin in the finished binary." OFF)
option(ENABLE_OPENGL "Build with EGL/OpenGL rendering support." ON)
option(TRY_ENABLE_OPENGL "Don't throw an error if EGL/OpenGL aren't found, instead just build without EGL/OpenGL support in that case." ON)
option(ENABLE_VULKAN "Build with Vulkan rendering support." OFF)
option(TRY_ENABLE_VULKAN "Don't throw an error if vulkan isn't found, instead just build without vulkan support in that case." OFF)
set(VULKAN_DEBUG "AUTO" CACHE STRING "Enable vulkan validation layers and verbose vulkan logging. (ON/OFF/AUTO)")
set_property(CACHE VULKAN_DEBUG PROPERTY STRINGS ON OFF AUTO)
option(ENABLE_SOFTWARE "Build with software rendering support." ON)
option(DUMP_ENGINE_LAYERS "True if flutter-drm-embedder should dump the list of rendering layers that the flutter engine sends to flutter-drm-embedder on each draw." OFF)
option(WARN_MISSING_FIELD_INITIALIZERS "True of the compiler should be instructed to warn about missing field initializers. This needs some hacky workarounds in the code so gcc won't report spurious warnings, so this should only be enabled if the warnings are explicitly required." OFF)
option(ENABLE_TSAN "True to build & link with -fsanitize=thread" OFF)
option(ENABLE_ASAN "True to build & link with -fsanitize=address" OFF)
option(ENABLE_UBSAN "True to build & link with -fsanitize=undefined" OFF)
option(ENABLE_MTRACE "True if flutter-drm-embedder should call GNU mtrace() on startup." OFF)
option(ENABLE_TESTS "True if tests should be built. Requires Unity to be checked out at third_party/Unity." OFF)
option(ENABLE_SESSION_SWITCHING "True if flutter-drm-embedder should be built with session switching support. Requires libseat-dev to be installed." ON)
option(TRY_ENABLE_SESSION_SWITCHING "Don't throw an error if libseat isn't found, instead just build without session switching support in that case." ON)
option(LTO "Check for IPO/LTO support and enable, if supported. May require gold/lld when building with clang. (Either using `-fuse-ld` in CMAKE_C_FLAGS or by setting as the default system linker.) Only applies to Release or RelWithDebInfo build types." ON)
option(LINT_EGL_HEADERS "Set an define that'll make the egl.h only export the extension definitions, prototypes that are explicitly marked as required." OFF)
option(DEBUG_DRM_PLANE_ALLOCATIONS "Add logging in modesetting.c for debugging the process of choosing a fitting DRM plane for a framebuffer layer." OFF)
option(USE_LEGACY_KMS "Force the use of legacy KMS." OFF)

# This is a CMake recognized variable, but we set it to off by default here.
option(CMAKE_POSITION_INDEPENDENT_CODE "Enable/Disable Position Independent Code" OFF)

set(FILESYSTEM_LAYOUTS default meta-flutter)
set(FILESYSTEM_LAYOUT "default" CACHE STRING "Where to look for the icudtl.dat, app.so/libapp.so, flutter asset bundle.")
set_property(CACHE FILESYSTEM_LAYOUT PROPERTY STRINGS ${FILESYSTEM_LAYOUTS})

set(SENTRY_BACKEND "crashpad" CACHE STRING "What sentry backend to use, when the sentry plugin is built. Allowed values are inproc, crashpad and breakpad.")
option(SENTRY_PLUGIN_BUNDLE_CRASHPAD_HANDLER "Bundle the crashpad_handler with the flutter-drm-embedder executable." ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release.")
endif()

if (BUILD_SENTRY_PLUGIN)
  set(flutter_drm_embedder_languages C CXX ASM)
else()
  set(flutter_drm_embedder_languages C ASM)
endif()

project(flutter-drm-embedder LANGUAGES ${flutter_drm_embedder_languages} VERSION "1.0.0")

# Version string: accept -DFLUTTER_DRM_EMBEDDER_VERSION=... from CI, otherwise
# derive from git tags (release/X.Y.Z -> X.Y.Z) + commit info.
if (NOT DEFINED FLUTTER_DRM_EMBEDDER_VERSION OR FLUTTER_DRM_EMBEDDER_VERSION STREQUAL "")
  execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_DESCRIBE_RESULT
  )
  if (GIT_DESCRIBE_RESULT EQUAL 0)
    # Strip "release/" prefix if present
    string(REGEX REPLACE "^release/" "" FLUTTER_DRM_EMBEDDER_VERSION "${GIT_DESCRIBE}")
  else()
    set(FLUTTER_DRM_EMBEDDER_VERSION "unknown")
  endif()
endif()
message(STATUS "flutter-drm-embedder version: ${FLUTTER_DRM_EMBEDDER_VERSION}")

if (BUILD_OPENAUTOFLUTTER_PLUGIN)
  enable_language(CXX)
endif()

message(STATUS "Generator .............. ${CMAKE_GENERATOR}")
message(STATUS "Build Type ............. ${CMAKE_BUILD_TYPE}")

include(CheckCCompilerFlag)

# Those libraries we definitely need.
include(FindPkgConfig)
pkg_check_modules(DRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(GBM REQUIRED IMPORTED_TARGET gbm)
pkg_check_modules(LIBSYSTEMD REQUIRED IMPORTED_TARGET libsystemd)
pkg_check_modules(LIBINPUT REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(LIBXKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(LIBUDEV REQUIRED IMPORTED_TARGET libudev)
pkg_check_modules(GLIB IMPORTED_TARGET glib-2.0)
pkg_check_modules(GOBJECT IMPORTED_TARGET gobject-2.0)
pkg_check_modules(GIO IMPORTED_TARGET gio-2.0)
pkg_check_modules(GTK IMPORTED_TARGET gtk+-3.0)
if (BUILD_OPENAUTOFLUTTER_PLUGIN)
  pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
endif()

# find pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(POLICY CMP0083)
  cmake_policy(SET CMP0083 NEW)

  include(CheckPIESupported)
  check_pie_supported()
endif()

message(STATUS "PIE .................... ${CMAKE_POSITION_INDEPENDENT_CODE}")

# flutter-drm-embedder executable
add_executable(
  flutter-drm-embedder
  src/main.c
)

# flutter_drm_embedder_module
# We separate the actual flutter-drm-embedder code into a separate object library
# so we can link against it in the tests.
add_library(
  flutter_drm_embedder_module OBJECT
  src/flutter-drm-embedder.c
  src/platformchannel.c
  src/pluginregistry.c
  src/texture_registry.c
  src/modesetting.c
  src/util/collection.c
  src/util/bitscan.c
  src/util/vector.c
  src/cursor.c
  src/keyboard.c
  src/user_input.c
  src/locales.c
  src/notifier_listener.c
  src/pixel_format.c
  src/filesystem_layout.c
  src/compositor_ng.c
  src/surface.c
  src/render_surface.c
  src/tracer.c
  src/dmabuf_surface.c
  src/frame_scheduler.c
  src/window.c
  src/dummy_render_surface.c
  src/plugins/services.c
)

if (BUILD_OPENAUTOFLUTTER_PLUGIN)
  target_sources(flutter_drm_embedder_module PRIVATE src/flutter_linux_gtk_shim/openautoflutter_registrant.c)
  target_compile_definitions(flutter_drm_embedder_module PUBLIC BUILD_OPENAUTOFLUTTER_PLUGIN)
endif()

if (BUILD_GTK_SHIM_TEST_PLUGIN)
  target_sources(flutter_drm_embedder_module PRIVATE src/flutter_linux_gtk_shim/shim_test_plugin.c)
  target_compile_definitions(flutter_drm_embedder_module PUBLIC BUILD_GTK_SHIM_TEST_PLUGIN)
endif()

target_link_libraries(flutter_drm_embedder_module PUBLIC
  PkgConfig::DRM
  PkgConfig::GBM
  PkgConfig::LIBSYSTEMD
  PkgConfig::LIBINPUT
  PkgConfig::LIBXKBCOMMON
  PkgConfig::LIBUDEV
  Threads::Threads
  ${CMAKE_DL_LIBS}
  rt m atomic
)

target_include_directories(flutter_drm_embedder_module PUBLIC
  ${CMAKE_SOURCE_DIR}/third_party/flutter_embedder_header/include
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}
)

if (BUILD_OPENAUTOFLUTTER_PLUGIN)
  target_include_directories(flutter_drm_embedder_module PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/flutter_linux_gtk_shim/include
    ${OPENAUTOFLUTTER_DIR}/linux/include
  )
  target_link_libraries(flutter_drm_embedder_module PUBLIC
    PkgConfig::GLIB
    PkgConfig::GOBJECT
    PkgConfig::GIO
  )
  if (GTK_FOUND)
    target_link_libraries(flutter_drm_embedder_module PUBLIC PkgConfig::GTK)
  endif()
endif()

target_compile_options(flutter_drm_embedder_module PUBLIC
  $<$<CONFIG:Debug>:-O0 -Wall -Wextra -Wno-sign-compare -Werror -ggdb -U_FORTIFY_SOURCE -DDEBUG>
  $<$<CONFIG:RelWithDebInfo>:-O3 -Wall -Wextra -Wno-sign-compare -ggdb -DNDEBUG>
  $<$<CONFIG:Release>:-O3 -Wall -Wextra -Wno-sign-compare -DNDEBUG>
)

# GTK-style plugin shim library for Linux plugins (always enabled)
if (NOT GLIB_FOUND OR NOT GOBJECT_FOUND OR NOT GIO_FOUND)
  message(SEND_ERROR "GLib/GObject/GIO not found. Install libglib2.0-dev and libgobject-2.0-dev (and gio).")
endif()

add_library(
  flutter_linux_gtk_shim
  SHARED
  src/flutter_linux_gtk_shim/fl_binary_messenger.c
  src/flutter_linux_gtk_shim/fl_method_codec.c
  src/flutter_linux_gtk_shim/fl_standard_method_codec.c
  src/flutter_linux_gtk_shim/fl_value.c
  src/flutter_linux_gtk_shim/fl_method_call.c
  src/flutter_linux_gtk_shim/fl_method_response.c
  src/flutter_linux_gtk_shim/fl_method_channel.c
  src/flutter_linux_gtk_shim/fl_event_channel.c
  src/flutter_linux_gtk_shim/fl_texture_registrar.c
  src/flutter_linux_gtk_shim/fl_texture_gl.c
  src/flutter_linux_gtk_shim/fl_plugin_registrar.c
  src/flutter_linux_gtk_shim/fl_plugin_registry.c
  src/flutter_linux_gtk_shim/fl_plugin_registrant.c
  src/flutter_linux_gtk_shim/fl_glib_dispatch.c
  src/plugin_loader.c
)

set_target_properties(flutter_linux_gtk_shim PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  OUTPUT_NAME flutter_linux_gtk
)

target_link_libraries(flutter_linux_gtk_shim PUBLIC
  PkgConfig::DRM
  PkgConfig::GLIB
  PkgConfig::GOBJECT
  PkgConfig::GIO
)

target_include_directories(flutter_linux_gtk_shim PUBLIC
  ${CMAKE_SOURCE_DIR}/third_party/flutter_linux_gtk_shim/include
  ${CMAKE_SOURCE_DIR}/third_party/flutter_embedder_header/include
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}
)

target_compile_options(flutter_linux_gtk_shim PUBLIC
  $<$<CONFIG:Debug>:-O0 -Wall -Wextra -Wno-sign-compare -Werror -ggdb -U_FORTIFY_SOURCE -DDEBUG>
  $<$<CONFIG:RelWithDebInfo>:-O3 -Wall -Wextra -Wno-sign-compare -ggdb -DNDEBUG>
  $<$<CONFIG:Release>:-O3 -Wall -Wextra -Wno-sign-compare -DNDEBUG>
)




target_link_libraries(flutter-drm-embedder PUBLIC flutter_linux_gtk_shim)
target_link_options(flutter-drm-embedder PUBLIC LINKER:--export-dynamic)

# OpenAutoFlutter plugin build (uses GTK-style shim)
if (BUILD_OPENAUTOFLUTTER_PLUGIN)
  if (NOT EXISTS ${OPENAUTOFLUTTER_DIR}/linux/openautoflutter_plugin.cc)
    message(SEND_ERROR "OpenAutoFlutter plugin sources not found. Set OPENAUTOFLUTTER_DIR correctly.")
  endif()

  add_library(openautoflutter_plugin SHARED
    ${OPENAUTOFLUTTER_DIR}/linux/openautoflutter_plugin.cc
    ${OPENAUTOFLUTTER_DIR}/linux/oat_message_handlers.cpp
    ${OPENAUTOFLUTTER_DIR}/linux/av/oa_video_texture.cc
    ${OPENAUTOFLUTTER_DIR}/linux/common/SharedMemoryConsumer.cpp
    ${OPENAUTOFLUTTER_DIR}/linux/av/h264_decoder.cc
  )

  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "OpenAutoTransport prebuilt libs are built with clang/libc++; consider configuring with CC=clang CXX=clang++ if you hit ABI/linker errors.")
  endif()

  set_target_properties(openautoflutter_plugin PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
  target_compile_definitions(openautoflutter_plugin PRIVATE FLUTTER_PLUGIN_IMPL)

  target_include_directories(openautoflutter_plugin PRIVATE
    ${OPENAUTOFLUTTER_DIR}/linux/include
    ${OPENAUTOFLUTTER_DIR}/linux/av
    ${OPENAUTOFLUTTER_DIR}/linux/common
    ${CMAKE_SOURCE_DIR}/third_party/flutter_linux_gtk_shim/include
  )

  target_link_libraries(openautoflutter_plugin PRIVATE flutter_linux_gtk_shim PkgConfig::EPOXY Threads::Threads ${CMAKE_DL_LIBS})
  if (GTK_FOUND)
    target_link_libraries(openautoflutter_plugin PRIVATE PkgConfig::GTK)
  endif()

  # OpenAutoTransport (prebuilt) fetch + link
  set(OPENAUTO_TRANSPORT_VERSION "v0.0.24" CACHE STRING "OpenAutoTransport release tag" FORCE)
  if (NOT DEFINED OPENAUTO_TRANSPORT_VARIANT)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _proc)
    if (_proc MATCHES "x86_64|amd64")
      set(OPENAUTO_TRANSPORT_VARIANT "amd64_gnu")
    elseif (_proc MATCHES "aarch64|arm64")
      set(OPENAUTO_TRANSPORT_VARIANT "arm64_gnu")
    else()
      message(SEND_ERROR "Unsupported processor ${CMAKE_SYSTEM_PROCESSOR}; set OPENAUTO_TRANSPORT_VARIANT manually (amd64_gnu|amd64_musl|arm64_gnu|arm64_musl)")
    endif()
  endif()

  set(_oat_base_url "https://github.com/buzzcola3/OpenAutoTransport/releases/download/${OPENAUTO_TRANSPORT_VERSION}")
  set(_oat_prefix "${CMAKE_BINARY_DIR}/openautotransport/${OPENAUTO_TRANSPORT_VERSION}/prebuilt")
  file(MAKE_DIRECTORY "${_oat_prefix}/include" "${_oat_prefix}/lib")

  function(_oat_fetch _name _dest_dir _out_var)
    set(_url "${_oat_base_url}/${_name}")
    set(_dest "${_dest_dir}/${_name}")
    set(_needs_fetch TRUE)
    if (EXISTS "${_dest}")
      file(SIZE "${_dest}" _size)
      if (_size GREATER 0)
        set(_needs_fetch FALSE)
      else()
        file(REMOVE "${_dest}")
      endif()
    endif()
    if (_needs_fetch)
      file(DOWNLOAD "${_url}" "${_dest}" SHOW_PROGRESS STATUS _status)
      list(GET _status 0 _code)
      if (NOT _code EQUAL 0)
        message(SEND_ERROR "Failed to download ${_url}: ${_status}")
      endif()
      file(SIZE "${_dest}" _size_after)
      if (NOT _size_after GREATER 0)
        message(SEND_ERROR "Downloaded ${_url} but file is empty")
      endif()
    endif()
    set(${_out_var} "${_dest}" PARENT_SCOPE)
  endfunction()

  _oat_fetch("wire.hpp" "${_oat_prefix}/include" _oat_wire_hpp)
  _oat_fetch("transport.hpp" "${_oat_prefix}/include" _oat_transport_hpp)
  _oat_fetch("wire.capnp.h" "${_oat_prefix}/include" _oat_wire_capnp_h)
  _oat_fetch("wire.capnp.c++" "${_oat_prefix}/include" _oat_wire_capnp_cpp)

  _oat_fetch("libopen_auto_transport-${OPENAUTO_TRANSPORT_VARIANT}.a" "${_oat_prefix}/lib" _oat_lib_path)
  _oat_fetch("libcapnp-${OPENAUTO_TRANSPORT_VARIANT}.a" "${_oat_prefix}/lib" _oat_capnp_path)
  _oat_fetch("libkj-${OPENAUTO_TRANSPORT_VARIANT}.a" "${_oat_prefix}/lib" _oat_kj_path)

  add_library(open_auto_transport STATIC IMPORTED)
  set_target_properties(open_auto_transport PROPERTIES IMPORTED_LOCATION "${_oat_lib_path}")
  add_library(open_auto_transport_capnp STATIC IMPORTED)
  set_target_properties(open_auto_transport_capnp PROPERTIES IMPORTED_LOCATION "${_oat_capnp_path}")
  add_library(open_auto_transport_kj STATIC IMPORTED)
  set_target_properties(open_auto_transport_kj PROPERTIES IMPORTED_LOCATION "${_oat_kj_path}")

  target_include_directories(openautoflutter_plugin PRIVATE "${_oat_prefix}/include")
  target_sources(openautoflutter_plugin PRIVATE "${_oat_wire_capnp_cpp}")
  target_link_libraries(openautoflutter_plugin PRIVATE open_auto_transport open_auto_transport_capnp open_auto_transport_kj)

  # libc++ required by OpenAutoTransport prebuilt libs
  target_compile_options(openautoflutter_plugin PRIVATE -stdlib=libc++)
  target_link_options(openautoflutter_plugin PRIVATE -stdlib=libc++)
  target_link_libraries(openautoflutter_plugin PRIVATE c++ c++abi)

  # FFmpeg (libav) for software H264 decoding
  find_library(AVCODEC_LIB avcodec)
  find_library(AVUTIL_LIB avutil)
  find_library(SWSCALE_LIB swscale)
  if (AVCODEC_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
    target_link_libraries(openautoflutter_plugin PRIVATE ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
  endif()

  target_link_libraries(flutter-drm-embedder PUBLIC openautoflutter_plugin)
endif()

# GCC prior to 11.3 reports false positives for missing-field-initializers warning.
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_C_COMPILER_VERSION VERSION_LESS "11.3")
    target_compile_options(flutter_drm_embedder_module PUBLIC -Wno-missing-field-initializers)
  endif()
  target_link_options(flutter-drm-embedder PUBLIC LINKER:--build-id)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  target_link_options(flutter-drm-embedder PUBLIC LINKER:--build-id=sha1)
endif()

# libinput stuff
# There's no other way to query the libinput version (in code) somehow.
# So we need to roll our own libinput version macro
string(REPLACE "." ";" LIBINPUT_VERSION_AS_LIST ${LIBINPUT_VERSION})
list(GET LIBINPUT_VERSION_AS_LIST 0 LIBINPUT_VERSION_MAJOR)
list(GET LIBINPUT_VERSION_AS_LIST 1 LIBINPUT_VERSION_MINOR)
list(GET LIBINPUT_VERSION_AS_LIST 2 LIBINPUT_VERSION_PATCH)

# TODO: Just unconditionally define those, make them optional later
set(HAVE_KMS ON)
set(HAVE_GBM ON)
set(HAVE_FBDEV ON)

# OpenGL support
set(HAVE_EGL OFF)
set(HAVE_GLES2 OFF)
set(HAVE_EGL_GLES2 OFF)

pkg_check_modules(EGL IMPORTED_TARGET egl)
pkg_check_modules(GLES2 IMPORTED_TARGET glesv2)
if (ENABLE_OPENGL)
  if (EGL_FOUND AND GLES2_FOUND)
    target_sources(flutter_drm_embedder_module PRIVATE
      src/egl_gbm_render_surface.c
      src/gl_renderer.c
    )
    target_link_libraries(flutter_drm_embedder_module PUBLIC
      PkgConfig::EGL
      PkgConfig::GLES2
    )

    set(HAVE_EGL ON)
    set(HAVE_GLES2 ON)
    set(HAVE_EGL_GLES2 ON)
  elseif (TRY_ENABLE_OPENGL)
    message("EGL and/or OpenGL was not found. Flutter-drm-embedder will build without EGL/OpenGL rendering support.")
  else()
    message(SEND_ERROR "EGL and/or OpenGL was not found. Try building with `-DTRY_ENABLE_OPENGL=On` if you want to just disable EGL/OpenGL support in that case.")
  endif()
endif()

message(STATUS "EGL/GLES support ....... ${HAVE_EGL_GLES2}")

message(STATUS "Lint EGL headers ....... ${LINT_EGL_HEADERS}")

# Vulkan support
set(HAVE_VULKAN OFF)

pkg_check_modules(VULKAN IMPORTED_TARGET vulkan)
if (ENABLE_VULKAN)
  if (VULKAN_FOUND)
    target_sources(flutter_drm_embedder_module PRIVATE
      src/vk_gbm_render_surface.c
      src/vk_renderer.c
    )
    target_link_libraries(flutter_drm_embedder_module PUBLIC
      PkgConfig::VULKAN
    )

    set(HAVE_VULKAN ON)
  elseif (TRY_ENABLE_VULKAN)
    message("Vulkan was not found. Flutter-drm-embedder will build without vulkan rendering support.")
  else()
    message(SEND_ERROR "Vulkan was not found. Try building with `-DTRY_ENABLE_VULKAN=On` if you want to just disable vulkan support in that case.")
  endif()
endif()

message(STATUS "Vulkan support ......... ${HAVE_VULKAN}")

# We need at least one renderer
if (NOT HAVE_VULKAN AND NOT HAVE_EGL_GLES2)
  message(SEND_ERROR "At least one of the EGL/GLES2 and Vulkan backends must be enabled.")
endif()

# Filesystem Layout
# meta-flutter or normal flutter-drm-embedder.
# see src/filesystem_layout.c for details.
if(NOT FILESYSTEM_LAYOUT IN_LIST FILESYSTEM_LAYOUTS)
  message(FATAL_ERROR "FILESYSTEM_LAYOUT must be one of ${FILESYSTEM_LAYOUTS}")
endif()

message(STATUS "Filesystem Layout ...... ${FILESYSTEM_LAYOUT}")

# config.h takes the defines in form of #cmakedefine FILESYSTEM_LAYOUT_DEFAULT, ...
if(FILESYSTEM_LAYOUT STREQUAL default)
  set(FILESYSTEM_LAYOUT_DEFAULT ON)
elseif(FILESYSTEM_LAYOUT STREQUAL meta-flutter)
  set(FILESYSTEM_LAYOUT_METAFLUTTER ON)
endif()

# Session switching support (using libseat)
set(HAVE_LIBSEAT OFF)

if (ENABLE_SESSION_SWITCHING)
  if (TRY_ENABLE_SESSION_SWITCHING)
    pkg_check_modules(LIBSEAT IMPORTED_TARGET libseat)
  else()
    pkg_check_modules(LIBSEAT REQUIRED IMPORTED_TARGET libseat)
  endif()

  if (LIBSEAT_FOUND)
    target_link_libraries(flutter_drm_embedder_module PUBLIC PkgConfig::LIBSEAT)
    set(HAVE_LIBSEAT ON)
  else()
    message("libseat was not found. flutter-drm-embedder will be built without session switching support.")
  endif()
endif()

message(STATUS "Session switching ...... ${HAVE_LIBSEAT}")

# TODO: We actually don't need the compile definitions anymore, except for
# text input and raw keyboard plugin (because those have special treatment
# in flutter-drm-embedder.c)
if (BUILD_TEXT_INPUT_PLUGIN)
  target_sources(flutter_drm_embedder_module PRIVATE src/plugins/text_input.c)
endif()
if (BUILD_RAW_KEYBOARD_PLUGIN)
  target_sources(flutter_drm_embedder_module PRIVATE src/plugins/raw_keyboard.c)
endif()
if (BUILD_TEST_PLUGIN)
  target_sources(flutter_drm_embedder_module PRIVATE src/plugins/testplugin.c)
endif()
if (BUILD_GSTREAMER_VIDEO_PLAYER_PLUGIN)
  if (NOT HAVE_EGL_GLES2)
    message(NOTICE "EGL and OpenGL ES2 are required for gstreamer video player. Gstreamer video player plugin won't be build.")
  else()
    if (TRY_BUILD_GSTREAMER_VIDEO_PLAYER_PLUGIN)
      pkg_check_modules(LIBGSTREAMER IMPORTED_TARGET gstreamer-1.0)
      pkg_check_modules(LIBGSTREAMER_PLUGINS_BASE IMPORTED_TARGET gstreamer-plugins-base-1.0)
      pkg_check_modules(LIBGSTREAMER_APP IMPORTED_TARGET gstreamer-app-1.0)
      pkg_check_modules(LIBGSTREAMER_ALLOCATORS IMPORTED_TARGET gstreamer-allocators-1.0)
      pkg_check_modules(LIBGSTREAMER_VIDEO IMPORTED_TARGET gstreamer-video-1.0)
    else()
      pkg_check_modules(LIBGSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
      pkg_check_modules(LIBGSTREAMER_PLUGINS_BASE REQUIRED IMPORTED_TARGET gstreamer-plugins-base-1.0)
      pkg_check_modules(LIBGSTREAMER_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
      pkg_check_modules(LIBGSTREAMER_ALLOCATORS REQUIRED IMPORTED_TARGET gstreamer-allocators-1.0)
      pkg_check_modules(LIBGSTREAMER_VIDEO REQUIRED IMPORTED_TARGET gstreamer-video-1.0)
    endif()

    if (LIBGSTREAMER_FOUND AND LIBGSTREAMER_PLUGINS_BASE_FOUND AND LIBGSTREAMER_APP_FOUND AND LIBGSTREAMER_ALLOCATORS_FOUND AND LIBGSTREAMER_VIDEO_FOUND)
      # There's no other way to query the libinput version (in code) somehow.
      # So we need to roll our own libinput version macro
      string(REPLACE "." ";" LIBGSTREAMER_VERSION_AS_LIST ${LIBGSTREAMER_VERSION})
      list(GET LIBGSTREAMER_VERSION_AS_LIST 0 LIBGSTREAMER_VERSION_MAJOR)
      list(GET LIBGSTREAMER_VERSION_AS_LIST 1 LIBGSTREAMER_VERSION_MINOR)
      list(GET LIBGSTREAMER_VERSION_AS_LIST 2 LIBGSTREAMER_VERSION_PATCH)

      target_sources(flutter_drm_embedder_module PRIVATE
        src/plugins/gstreamer_video_player/plugin.c
        src/plugins/gstreamer_video_player/player.c
        src/plugins/gstreamer_video_player/frame.c
      )
      target_link_libraries(flutter_drm_embedder_module PUBLIC
        PkgConfig::LIBGSTREAMER
        PkgConfig::LIBGSTREAMER_PLUGINS_BASE
        PkgConfig::LIBGSTREAMER_APP
        PkgConfig::LIBGSTREAMER_ALLOCATORS
        PkgConfig::LIBGSTREAMER_VIDEO
      )
    else()
      message(NOTICE "Couldn't find gstreamer libraries. Gstreamer video player plugin won't be build.")
    endif()
  endif()
endif()

if (BUILD_GSTREAMER_AUDIO_PLAYER_PLUGIN)
  if (TRY_BUILD_GSTREAMER_AUDIO_PLAYER_PLUGIN)
    pkg_check_modules(LIBGSTREAMER IMPORTED_TARGET gstreamer-1.0)
    pkg_check_modules(LIBGSTREAMER_APP IMPORTED_TARGET gstreamer-app-1.0)
    pkg_check_modules(LIBGSTREAMER_AUDIO IMPORTED_TARGET gstreamer-audio-1.0)
  else()
    pkg_check_modules(LIBGSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
    pkg_check_modules(LIBGSTREAMER_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
    pkg_check_modules(LIBGSTREAMER_AUDIO REQUIRED IMPORTED_TARGET gstreamer-audio-1.0)
  endif()

  if (LIBGSTREAMER_FOUND AND LIBGSTREAMER_APP_FOUND AND LIBGSTREAMER_AUDIO_FOUND)
    target_sources(flutter_drm_embedder_module PRIVATE
      src/plugins/audioplayers/plugin.c
      src/plugins/audioplayers/player.c
    )
    target_link_libraries(flutter_drm_embedder_module PUBLIC
      PkgConfig::LIBGSTREAMER
      PkgConfig::LIBGSTREAMER_APP
      PkgConfig::LIBGSTREAMER_AUDIO
    )
  else()
    message(NOTICE "Couldn't find gstreamer libraries. Gstreamer audio player plugin won't be build.")
  endif()
endif()

if (BUILD_CHARSET_CONVERTER_PLUGIN)
  target_sources(flutter_drm_embedder_module PRIVATE src/plugins/charset_converter.c)
endif()

# Sentry Plugin
set(HAVE_BUNDLED_CRASHPAD_HANDLER OFF)
if (BUILD_SENTRY_PLUGIN)
  set(SENTRY_PIC ${CMAKE_POSITION_INDEPENDENT_CODE} CACHE BOOL "")
  add_subdirectory(third_party/sentry-native)

  target_sources(flutter_drm_embedder_module PRIVATE src/plugins/sentry/sentry.c)
  target_link_libraries(flutter_drm_embedder_module PUBLIC sentry::sentry)

  if (SENTRY_BACKEND STREQUAL "crashpad" AND SENTRY_PLUGIN_BUNDLE_CRASHPAD_HANDLER)
    set(HAVE_BUNDLED_CRASHPAD_HANDLER ON)
  
    target_sources(flutter-drm-embedder PRIVATE src/crashpad_handler_trampoline.cc)
    # link against the same libraries the crashpad_handler uses
    
    get_target_property(handler_deps crashpad_handler INTERFACE_LINK_LIBRARIES)
    target_link_libraries(flutter-drm-embedder PUBLIC ${handler_deps})
  endif()
endif()
message(STATUS "Sentry plugin .......... ${BUILD_SENTRY_PLUGIN}")
message(STATUS "Bundle crashpad_handler  ${HAVE_BUNDLED_CRASHPAD_HANDLER}")

# Needed so dart VM can actually resolve symbols in the same
# executable. (For dart:ffi DynamicLibrary.executable / DynamicLibrary.process)
target_link_options(flutter_drm_embedder_module PUBLIC -rdynamic)

# Define VULKAN_DEBUG if it was set to On, or if it was set to AUTO and we're using debug mode.
if (VULKAN_DEBUG MATCHES AUTO)
  if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(VULKAN_DEBUG ON)
  else()
    set(VULKAN_DEBUG OFF)
  endif()
elseif (VULKAN_DEBUG MATCHES "OFF")
  set(VULKAN_DEBUG OFF)
elseif (VULKAN_DEBUG MATCHES "ON")
  set(VULKAN_DEBUG ON)
endif()

# Some sanitizer configs.
if (ENABLE_TSAN)
  target_link_options(flutter_drm_embedder_module PUBLIC -fsanitize=thread)
  target_compile_options(flutter_drm_embedder_module PUBLIC -fsanitize=thread)
endif()
if (ENABLE_ASAN)
  # when we use asan, we need to force linking against the C++ stdlib.
  # If we don't link against it, and load a dynamic library that's linked against the C++ stdlib (like the flutter engine),
  # and something in the dynamically loaded library triggers asan, it'll throw an error because it hasn't
  # intercepted stdc++ yet.
  # Also disable --as-needed so we _actually_ link against c++, even though we don't use any symbols from it.
  target_link_libraries(flutter_drm_embedder_module PUBLIC stdc++)
  target_link_options(flutter_drm_embedder_module PUBLIC -fsanitize=address -fno-omit-frame-pointer -Wl,--no-as-needed)
  target_compile_options(flutter_drm_embedder_module PUBLIC -fsanitize=address)

  check_c_compiler_flag(-static-libasan HAVE_STATIC_LIBASAN)
  if (HAVE_STATIC_LIBASAN)
    target_link_options(flutter_drm_embedder_module PUBLIC -static-libasan)
  endif()
endif()
if (ENABLE_UBSAN)
  target_link_options(flutter_drm_embedder_module PUBLIC -fsanitize=undefined)
  target_compile_options(flutter_drm_embedder_module PUBLIC -fsanitize=undefined)
endif()

configure_file(config.h.in config.h @ONLY)

# TODO: make mapping portable
# is mapping `../src/` to `` always gonna work?
check_c_compiler_flag(-fmacro-prefix-map=.=. COMPILER_SUPPORTS_MACRO_PREFIX_MAP)
if (COMPILER_SUPPORTS_MACRO_PREFIX_MAP)
  target_compile_options(flutter_drm_embedder_module PRIVATE "-fmacro-prefix-map=../src/=")
  target_compile_options(flutter_drm_embedder_module PRIVATE "-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/src/=")
endif()

# Actual flutter-drm-embedder executable.
target_link_libraries(
  flutter-drm-embedder PUBLIC
  flutter_drm_embedder_module
)
install(TARGETS flutter-drm-embedder RUNTIME DESTINATION bin)
install(TARGETS flutter_linux_gtk_shim LIBRARY DESTINATION bin)

# Enable lto if supported.
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
# include(CheckLinkerFlag)

set(USE_LTO OFF)
# set(NEEDS_GOLD OFF)
# set(NEEDS_LLD OFF)
if(LTO AND (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
  # So people can specify `-fuse-ld=lld` in the CMAKE_C_FLAGS.
  # Otherwise check_ipo_supported will not use CMAKE_C_FLAGS.
  if (POLICY CMP0138)
    cmake_policy(SET CMP0138 NEW)
  endif()

  check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_SUPPORT_OUTPUT)
  if (NOT IPO_SUPPORTED)
    message(WARNING "IPO/LTO was requested in the configure options, but is not supported by the toolchain. Check CMakeFiles/CMakeError.log for details.")
  endif()

  # Try to enable IPO with gold and lld.
  # Needs CMP0138.
  # (untested because CMP0138 required CMake 3.24, that's why it's commented out)
  # if (NOT IPO_SUPPORTED)
  #   check_linker_flag(C "-fuse-ld=gold" SUPPORTS_GOLD)
  #   if (SUPPORTS_GOLD)
  #     set(OLD_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  #
  #     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=gold")
  #     try_compile()
  #     check_ipo_supported(RESULT IPO_SUPPORTED_WITH_GOLD OUTPUT IPO_SUPPORT_OUTPUT)
  #     if (IPO_SUPPORTED_WITH_GOLD)
  #       set(IPO_SUPPORTED ON)
  #       set(NEEDS_GOLD ON)
  #     endif()
  #
  #     set(CMAKE_C_FLAGS "${OLD_CMAKE_C_FLAGS}")
  #   endif()
  #
  #   check_linker_flag(C "-fuse-ld=lld" SUPPORTS_LLD)
  #   if (SUPPORTS_LLD)
  #     set(OLD_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  #
  #     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=lld")
  #     check_ipo_supported(RESULT IPO_SUPPORTED_WITH_LLD OUTPUT IPO_SUPPORT_OUTPUT)
  #     if (IPO_SUPPORTED_WITH_LLD)
  #       set(IPO_SUPPORTED ON)
  #       set(NEEDS_LLD ON)
  #     endif()
  #
  #     set(CMAKE_C_FLAGS "${OLD_CMAKE_C_FLAGS}")
  #   endif()
  # endif()

  # clang doesn't support LTO when using GNU ld.
  if(IPO_SUPPORTED AND ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    execute_process(COMMAND ${CMAKE_C_COMPILER} -Wl,--version OUTPUT_VARIABLE LINKER_VERSION_OUTPUT ERROR_QUIET)
    if("${LINKER_VERSION_OUTPUT}" MATCHES "GNU ld")
      message(WARNING "IPO/LTO was requested, but is not supported when using clang with GNU ld as the linker. Try setting gold or lld as the system linker.")
      set(IPO_SUPPORTED OFF)
    endif()
  endif()

  if (IPO_SUPPORTED)
    set(USE_LTO ON)
  endif()
endif()

message(STATUS "IPO/LTO ................ ${USE_LTO}")
if (USE_LTO)
  set_property(TARGET flutter_drm_embedder_module PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_property(TARGET flutter-drm-embedder PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  # if (NEEDS_GOLD)
  #   Technically specifying only for one would suffice.
  #   target_link_options(flutter_drm_embedder_module PUBLIC "-fuse-ld=gold")
  #   target_link_options(flutter-drm-embedder PUBLIC "-fuse-ld=gold")
  # elseif (NEEDS_LLD)
  #   target_link_options(flutter_drm_embedder_module PUBLIC "-fuse-ld=lld")
  #   target_link_options(flutter-drm-embedder PUBLIC "-fuse-ld=lld")
  # endif()
endif()

if(ENABLE_TESTS)
  include(CTest)

  add_subdirectory(third_party)
  add_subdirectory(test)
endif()
