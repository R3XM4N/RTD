cmake_minimum_required(VERSION 3.5)
project(RTD)

set(CMAKE_C_STANDARD 11)

# Include directories for headers
include_directories(include)

# Add source files
add_executable(RTD src/main.c src/sdl.c src/system.c)

# Find and link SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(RTD ${SDL2_LIBRARIES})

# Find and link SDL2_image
find_package(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(RTD ${SDL2_IMAGE_LIBRARIES})

# Find and link SDL2_ttf
find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(RTD ${SDL2_TTF_LIBRARIES})

# Find and link SDL2_mixer
find_package(SDL2_mixer REQUIRED)
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
target_link_libraries(RTD ${SDL2_MIXER_LIBRARIES})

# Link the math library
target_link_libraries(RTD m)

# Ensure proper linking of all SDL libraries
target_link_libraries(RTD SDL2 SDL2_image SDL2_ttf SDL2_mixer)
