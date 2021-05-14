if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(node_audio LANGUAGES CXX VERSION 1.0.0)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0 ")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
endif()

set(CMAKE_BUILD_TYPE Debug)

include(NodeJS.cmake)
nodejs_init()
set(CMAKE_CXX_STANDARD 17)

IF (WIN32)
    # Finding ffmpeg on windows is a shitshow, so we just use precompiled libs
    set(FFMPEG_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/lib/ffmpeg/include)
    set(FFMPEG_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/ffmpeg/lib)
    set(FFMPEG_LIBRARIES
            ${FFMPEG_LIBRARY_DIR}/avformat.lib
            ${FFMPEG_LIBRARY_DIR}/avcodec.lib
            ${FFMPEG_LIBRARY_DIR}/avutil.lib
            ${FFMPEG_LIBRARY_DIR}/swresample.lib
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
    pkg_search_module(LIBAVUTIL REQUIRED libavutil)
    pkg_search_module(LIBSWRESAMPLE REQUIRED libswresample)

    set(FFMPEG_INCLUDE_DIRS ${LIBAVFORMAT_INCLUDE_DIRS} ${LIBAVCODEC_INCLUDE_DIRS})
    set(FFMPEG_LIBRARY_DIR ${LIBAVFORMAT_LIBRARY_DIRS} ${LIBAVCODEC_LIBRARY_DIRS})
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARY_DIR}/${LIBAVFORMAT_LIBRARIES} ${FFMPEG_LIBRARY_DIR}/${LIBAVCODEC_LIBRARIES})
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static library")
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "Don't build tests")
if(WIN32)
    set(CMAKE_DEBUG_POSTFIX CACHE STRING "Postfix for debug version of library" FORCE)
endif()
add_subdirectory(lib/rtaudio)

add_nodejs_module(${PROJECT_NAME}
        src/node_audio.cpp
        src/interface/CommandWorker.cpp
        src/interface/CommandWorker.h
        src/interface/AudioPlayer.cpp
        src/interface/AudioPlayer.h
        src/implementation/AudioPlayerImpl.cpp
        src/implementation/AudioPlayerImpl.h
        src/implementation/FFSource.cpp
        src/implementation/FFSource.h
        src/structs/PromiseContainer.h
        src/structs/PlayerCommand.h
        src/structs/commands/LoadCommand.h
        src/enums/Command.h
        src/enums/CommandResult.h
        src/exceptions/CommandException.cpp
        src/exceptions/CommandException.h
)

execute_process(COMMAND node -p "require('node-addon-api').include" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS} rtaudio)

target_compile_options(rtaudio PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -w>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W0>
)

target_link_directories(${PROJECT_NAME} BEFORE PRIVATE ${FFMPEG_LIBRARY_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC src)

target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} rtaudio)
