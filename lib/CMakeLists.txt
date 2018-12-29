set(CORE_FILES Graph.h Graph.cpp Module.h Module.cpp AudioBuffer.h AudioBuffer.cpp GraphModule.h GraphModule.cpp)
add_library(dcAudioGraph ${CORE_FILES} ${MODULE_FILES})
target_include_directories(dcAudioGraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
