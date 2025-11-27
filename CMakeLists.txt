cmake_minimum_required(VERSION 4.0)
project(minesweeper C)

set(CMAKE_C_STANDARD 99)

set(SDL2_DIR "$ENV{USERPROFILE}/scoop/apps/sdl2/current")
set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")

# find_package(SDL2 REQUIRED SDL2)
set(SDL2_LIBRARIES "${SDL2_DIR}/lib/SDL2main.lib;${SDL2_DIR}/lib/SDL2.lib")
include_directories(minesweeper ${SDL2_INCLUDE_DIRS})

add_executable(minesweeper
        gui.c
        gui.h
        main.c
        map.c
        map.h
        args.c
        args.h
        filehandler.c
        filehandler.h
)
target_link_libraries(minesweeper ${SDL2_LIBRARIES})