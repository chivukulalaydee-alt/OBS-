include_guard(DIRECTORY)

option(ENABLE_MEDIA_WORKSHOP "Enable the built-in media workshop" ON)

if(NOT ENABLE_MEDIA_WORKSHOP)
  target_disable_feature(obs-studio "Built-in media workshop")
  return()
endif()

target_enable_feature(obs-studio "Built-in media workshop")
target_compile_definitions(obs-studio PRIVATE MEDIA_WORKSHOP_ENABLED)

target_sources(
  obs-studio
  PRIVATE
    media-workshop/FFmpegCommandBuilder.cpp
    media-workshop/FFmpegCommandBuilder.hpp
    media-workshop/FFmpegProgressParser.cpp
    media-workshop/FFmpegProgressParser.hpp
    media-workshop/FFmpegRunner.cpp
    media-workshop/FFmpegRunner.hpp
    media-workshop/MediaProbe.cpp
    media-workshop/MediaProbe.hpp
    media-workshop/MediaWorkshopDock.cpp
    media-workshop/MediaWorkshopDock.hpp
    media-workshop/MediaWorkshopJob.hpp
    media-workshop/MediaWorkshopQueue.cpp
    media-workshop/MediaWorkshopQueue.hpp
    media-workshop/MediaWorkshopTools.cpp
    media-workshop/MediaWorkshopTools.hpp
    media-workshop/PlaylistBridge.cpp
    media-workshop/PlaylistBridge.hpp
)

add_executable(
  media-workshop-logic-tests
  EXCLUDE_FROM_ALL
  media-workshop/FFmpegCommandBuilder.cpp
  media-workshop/FFmpegCommandBuilder.hpp
  media-workshop/FFmpegProgressParser.cpp
  media-workshop/FFmpegProgressParser.hpp
  media-workshop/MediaProbe.cpp
  media-workshop/MediaProbe.hpp
  media-workshop/MediaWorkshopJob.hpp
  media-workshop/tests/MediaWorkshopLogicTests.cpp
)
target_link_libraries(media-workshop-logic-tests PRIVATE Qt6::Core)
set_target_properties(media-workshop-logic-tests PROPERTIES FOLDER "tests")

add_executable(
  media-workshop-runner-tests
  EXCLUDE_FROM_ALL
  media-workshop/FFmpegCommandBuilder.cpp
  media-workshop/FFmpegCommandBuilder.hpp
  media-workshop/FFmpegProgressParser.cpp
  media-workshop/FFmpegProgressParser.hpp
  media-workshop/FFmpegRunner.cpp
  media-workshop/FFmpegRunner.hpp
  media-workshop/MediaProbe.cpp
  media-workshop/MediaProbe.hpp
  media-workshop/MediaWorkshopJob.hpp
  media-workshop/MediaWorkshopQueue.cpp
  media-workshop/MediaWorkshopQueue.hpp
  media-workshop/MediaWorkshopTools.cpp
  media-workshop/MediaWorkshopTools.hpp
  media-workshop/tests/MediaWorkshopRunnerTests.cpp
)
target_link_libraries(media-workshop-runner-tests PRIVATE OBS::libobs Qt6::Core)
set_target_properties(media-workshop-runner-tests PROPERTIES AUTOMOC ON FOLDER "tests")
