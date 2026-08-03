cmake_minimum_required(VERSION 3.28...3.30)

macro(check_vlc_path)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_Libvlc QUIET libvlc)
  endif()

  find_path(
    LibVLC_INCLUDE_DIR
    NAMES vlc/libvlc.h
    HINTS ${PC_Libvlc_INCLUDEDIR} ${VLC_PATH}/include
    PATHS /usr/include /usr/local/include
    DOC "LibVLC include directory"
  )

  if(LibVLC_INCLUDE_DIR)
    target_include_directories(vlc-video PRIVATE "${LibVLC_INCLUDE_DIR}")
  else()
    message(FATAL_ERROR "Required system header <vlc/libvlc.h> not found.")
  endif()
endmacro()

option(ENABLE_VLC "Build OBS with VLC plugin support" ON)
if(NOT ENABLE_VLC)
  target_disable(vlc-video)
  return()
endif()

set(VLC_RUNTIME_PATH "" CACHE PATH "Directory containing the bundled Windows libVLC runtime")

add_library(vlc-video MODULE)
add_library(OBS::vlc-video ALIAS vlc-video)

target_sources(vlc-video PRIVATE vlc-video-plugin.c vlc-video-plugin.h vlc-video-source.c)

target_compile_options(vlc-video PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>)

target_link_libraries(vlc-video PRIVATE OBS::libobs $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

check_vlc_path()

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in vlc-video.rc)
  target_sources(vlc-video PRIVATE vlc-video.rc)

  if(VLC_RUNTIME_PATH)
    set(vlc_runtime_destination "${OBS_DATA_DESTINATION}/obs-plugins/vlc-video/runtime")
    set(vlc_runtime_required_files libvlc.dll libvlccore.dll COPYING.txt obs-vlc-runtime.json)
    set(vlc_runtime_required_directories plugins lua locale hrtfs)

    foreach(vlc_runtime_file IN LISTS vlc_runtime_required_files)
      if(NOT EXISTS "${VLC_RUNTIME_PATH}/${vlc_runtime_file}")
        message(FATAL_ERROR "Bundled VLC runtime is missing ${VLC_RUNTIME_PATH}/${vlc_runtime_file}")
      endif()
    endforeach()

    foreach(vlc_runtime_directory IN LISTS vlc_runtime_required_directories)
      if(NOT IS_DIRECTORY "${VLC_RUNTIME_PATH}/${vlc_runtime_directory}")
        message(FATAL_ERROR "Bundled VLC runtime is missing ${VLC_RUNTIME_PATH}/${vlc_runtime_directory}")
      endif()
    endforeach()

    install(
      FILES "${VLC_RUNTIME_PATH}/libvlc.dll" "${VLC_RUNTIME_PATH}/libvlccore.dll"
            "${VLC_RUNTIME_PATH}/COPYING.txt" "${VLC_RUNTIME_PATH}/obs-vlc-runtime.json"
      DESTINATION "${vlc_runtime_destination}"
      COMPONENT Runtime
    )
    install(
      DIRECTORY "${VLC_RUNTIME_PATH}/plugins" "${VLC_RUNTIME_PATH}/lua" "${VLC_RUNTIME_PATH}/locale"
                "${VLC_RUNTIME_PATH}/hrtfs"
      DESTINATION "${vlc_runtime_destination}"
      COMPONENT Runtime
    )

    add_custom_command(
      TARGET vlc-video
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "Copy bundled VLC runtime to data directory"
      COMMAND "${CMAKE_COMMAND}" -E remove_directory
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${VLC_RUNTIME_PATH}/libvlc.dll"
              "${VLC_RUNTIME_PATH}/libvlccore.dll" "${VLC_RUNTIME_PATH}/COPYING.txt"
              "${VLC_RUNTIME_PATH}/obs-vlc-runtime.json"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}/"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${VLC_RUNTIME_PATH}/plugins"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}/plugins"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${VLC_RUNTIME_PATH}/lua"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}/lua"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${VLC_RUNTIME_PATH}/locale"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}/locale"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${VLC_RUNTIME_PATH}/hrtfs"
              "${OBS_OUTPUT_DIR}/$<CONFIG>/${vlc_runtime_destination}/hrtfs"
      COMMENT ""
      VERBATIM
    )
  endif()
endif()

set_target_properties_obs(vlc-video PROPERTIES FOLDER plugins PREFIX "")
