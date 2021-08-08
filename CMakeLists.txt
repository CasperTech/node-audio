if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(node_audio LANGUAGES CXX VERSION 1.0.0)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0 ")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
endif()

set(CMAKE_BUILD_TYPE Release)

include(NodeJS.cmake)
nodejs_init()
set(CMAKE_CXX_STANDARD 17)

IF (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Finding ffmpeg on windows is a shitshow, so we just use precompiled libs
    set(FFMPEG_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/lib/ffmpeg/include)
    set(FFMPEG_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/ffmpeg/lib)
    set(FFMPEG_LIBRARIES
            ${FFMPEG_LIBRARY_DIR}/avformat.lib
            ${FFMPEG_LIBRARY_DIR}/avcodec.lib
            ${FFMPEG_LIBRARY_DIR}/avutil.lib
            ${FFMPEG_LIBRARY_DIR}/swresample.lib
            ${FFMPEG_LIBRARY_DIR}/swscale.lib
            ${FFMPEG_LIBRARY_DIR}/avresample.lib
            ${FFMPEG_LIBRARY_DIR}/postproc.lib
            ${FFMPEG_LIBRARY_DIR}/avfilter.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/lame/libmp3lame-static.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/lame/libmpghip-static.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/ogg/ogg.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/opus/opus.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/speex/libspeex.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/vorbis/vorbis.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/vorbis/vorbisenc.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/fdk-aac/fdk-aac.lib
            ${CMAKE_CURRENT_LIST_DIR}/lib/wavpack/libwavpack.lib
            strmiids
            mfuuid
            mfplat
            bcrypt
            secur32
            ws2_32
            crypt32
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LIBAVFORMAT REQUIRED libavformat)
    pkg_search_module(LIBAVCODEC REQUIRED libavcodec)
    pkg_search_module(LIBAVFILTER REQUIRED libavfilter)
    pkg_search_module(LIBAVUTIL REQUIRED libavutil)
    pkg_search_module(LIBAVRESAMPLE REQUIRED libavresample)
    pkg_search_module(LIBSWRESAMPLE REQUIRED libswresample)
    pkg_search_module(LIBSWSCALE REQUIRED libswscale)
    pkg_search_module(LIBPOSTPROC REQUIRED libpostproc)

    set(PLATFORM_LIBRARIES atomic)
    set(FFMPEG_INCLUDE_DIRS ${LIBAVFORMAT_INCLUDE_DIRS} ${LIBAVCODEC_INCLUDE_DIRS} ${LIBAVFILTER_INCLUDE_DIRS} ${LIBAVRESAMPLE_INCLUDE_DIRS})
    set(FFMPEG_LIBRARY_DIR ${LIBAVFORMAT_LIBRARY_DIRS} ${LIBAVCODEC_LIBRARY_DIRS} ${LIBAVFILTER_LIBRARY_DIRS} ${LIBAVRESAMPLE_INCLUDE_DIRS})
    set(FFMPEG_LIBRARIES ${LIBAVFORMAT_LIBRARIES} ${LIBAVCODEC_LIBRARIES} ${LIBAVUTIL_LIBRARIES} ${LIBSWRESAMPLE_LIBRARIES} ${LIBAVRESAMPLE_LIBRARIES} ${LIBSWSCALE_LIBRARIES} ${LIBPOSTPROC_LIBRARIES} ${LIBAVFILTER_LIBRARIES})
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static library")
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "Don't build tests")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(RTAUDIO_STATIC_MSVCRT ON CACHE BOOL "Static RuntimeLibrary")
    set(CMAKE_DEBUG_POSTFIX CACHE STRING "Postfix for debug version of library" FORCE)
endif()

set(LIB_FILES
        src/interfaces/IAudioPlayerEventReceiver.h
        src/interfaces/IAudioRenderer.h
        src/interfaces/IAudioSink.h
        src/interfaces/IAudioSink.cpp
        src/interfaces/IAudioSource.h
        src/interfaces/IAudioSource.cpp
        src/interfaces/IAudioNode.h
        src/interfaces/IAudioNode.cpp
        src/implementation/ScopedPacketUnref.cpp
        src/implementation/ScopedPacketUnref.h
        src/implementation/RingBuffer.cpp
        src/implementation/RingBuffer.h
        src/implementation/ScopedNodeRef.h
        src/implementation/AudioPlayerImpl.cpp
        src/implementation/AudioPlayerImpl.h
        src/implementation/SampleRateConverter.cpp
        src/implementation/SampleRateConverter.h
        src/implementation/VolumeFilter.cpp
        src/implementation/VolumeFilter.h
        src/implementation/RtAudioRenderer.cpp
        src/implementation/RtAudioRenderer.h
        src/implementation/RtAudioStream.cpp
        src/implementation/RtAudioStream.h
        src/implementation/FFSource.cpp
        src/implementation/FFSource.h
        src/implementation/FFFrame.h
        src/implementation/AudioCallbackContainer.h
        src/structs/events/CommandEvent.h
        src/structs/PlayerEvent.h
        src/structs/commands/LoadCommand.h
        src/structs/commands/PlayCommand.h
        src/structs/commands/StopCommand.h
        src/structs/commands/SeekCommand.h
        src/structs/commands/PauseCommand.h
        src/structs/commands/SetVolumeCommand.h
        src/structs/events/PlaybackFinishedEvent.h
        src/structs/events/PlaybackErrorEvent.h
        src/structs/events/PlayingEvent.h
        src/enums/Command.h
        src/enums/CommandResult.h
        src/enums/PlayerState.h
        src/enums/EventType.h
        src/enums/PlaybackEvent.h
        src/enums/SampleFormatFlags.h
        src/enums/AudioError.h
        src/exceptions/CommandException.cpp
        src/exceptions/CommandException.h
        src/exceptions/PlayerException.cpp
        src/exceptions/PlayerException.h
        src/exceptions/AudioException.cpp
        src/exceptions/AudioException.h
)

add_nodejs_module(${PROJECT_NAME}
        src/node_audio.cpp
        src/interface/CommandWorker.cpp
        src/interface/CommandWorker.h
        src/interface/AudioPlayer.cpp
        src/interface/AudioPlayer.h
        ${LIB_FILES}
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

add_subdirectory(lib/rtaudio)

execute_process(COMMAND node -p "require('node-addon-api').include" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/lib/rtaudio)

target_compile_options(rtaudio PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -w>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W0>
)

target_link_directories(${PROJECT_NAME} BEFORE PRIVATE ${FFMPEG_LIBRARY_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC src)

target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} rtaudio ${PLATFORM_LIBRARIES})

add_executable(Test test.cpp ${LIB_FILES})
target_include_directories(Test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(Test SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS}  ${CMAKE_CURRENT_LIST_DIR}/lib/rtaudio)
target_link_libraries(Test ${FFMPEG_LIBRARIES} rtaudio)