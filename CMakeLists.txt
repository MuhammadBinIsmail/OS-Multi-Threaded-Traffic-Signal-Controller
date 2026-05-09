cmake_minimum_required(VERSION 3.10)
project(TrafficSignalController C)

set(CMAKE_C_STANDARD 99)

find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    src
    gui
)

add_executable(traffic
    src/main.c
    src/shared.c
    src/lane.c
    src/stats.c
    src/intersection.c
    src/vehicle_gen.c
    src/emergency.c
    gui/renderer.c
)

target_link_libraries(traffic
    ${SDL2_LIBRARIES}
    Threads::Threads
    SDL2_ttf
    m
)