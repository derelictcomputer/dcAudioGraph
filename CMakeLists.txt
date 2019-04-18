cmake_minimum_required(VERSION 3.14)
project(dcAudioGraph)

set(CMAKE_CXX_STANDARD 14)

include_directories(dcAudioGraph)
include_directories(test)

set(SRC dcAudioGraph/AudioBuffer.h
        dcAudioGraph/AudioBuffer.cpp
        dcAudioGraph/EventBuffer.h
        dcAudioGraph/EventBuffer.cpp
        dcAudioGraph/Gain.h
        dcAudioGraph/Gain.cpp
        dcAudioGraph/Graph.h
        dcAudioGraph/Graph.cpp
        dcAudioGraph/LevelMeter.h
        dcAudioGraph/LevelMeter.cpp
        dcAudioGraph/MessageQueue.h
        dcAudioGraph/Module.h
        dcAudioGraph/Module.cpp
        dcAudioGraph/ModuleParam.h
        dcAudioGraph/Module.cpp)

add_library(dcAudioGraph STATIC ${SRC})

# Tests
set(SRC test/Test_Common.h
        test/Test_Common.cpp
        test/Test_Buffer.cpp
        test/Test_Graph.cpp)

add_executable(dcAudioGraph-test ${SRC})
target_link_libraries(dcAudioGraph-test dcAudioGraph)
