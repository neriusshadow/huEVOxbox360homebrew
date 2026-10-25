cmake_minimum_required(VERSION 3.15)
project(huEVO_Xbox360 CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

set(SOURCES
    src/main.cpp
    src/app.cpp
    src/renderer.cpp
    src/input.cpp
    src/network.cpp
    src/tls_client.cpp
    src/keyboard.cpp
    src/ui.cpp
)

set(HEADERS
    src/app.h
    src/config.h
    src/input.h
    src/keyboard.h
    src/models.h
    src/network.h
    src/tls_client.h
    src/renderer.h
    src/ui.h
)

if(DEFINED XBOX OR CMAKE_SYSTEM_NAME MATCHES "Xbox")
    add_definitions(-D_XBOX -DXBOX)
    add_executable(huEVO ${SOURCES} ${HEADERS})
    target_link_libraries(huEVO d3d9 xgraphics xnet xapilib)
    set_target_properties(huEVO PROPERTIES OUTPUT_NAME "default.xex")
else()
    message(STATUS "Configuring huEVO for Host Desktop Preview / Validation")
    add_executable(huEVO_test ${SOURCES} ${HEADERS})
endif()
