cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lilly)

# Run image conversion script before build
set(IMG_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/convert_images.py)
set(IMG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/img)
set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/generated)
set(GEN_C ${GEN_DIR}/images.c)
set(GEN_H ${GEN_DIR}/images.h)

# Find all source images
file(GLOB IMG_FILES ${IMG_DIR}/*.jpg ${IMG_DIR}/*.jpeg ${IMG_DIR}/*.png)

# Custom command to generate image arrays
add_custom_command(
    OUTPUT ${GEN_C} ${GEN_H}
    COMMAND ${Python3_EXECUTABLE} ${IMG_SCRIPT}
    DEPENDS ${IMG_SCRIPT} ${IMG_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Converting images to RGB565 arrays"
)

# Custom target for image generation
add_custom_target(generate_images DEPENDS ${GEN_C} ${GEN_H})

# Make app depend on generated images
add_dependencies(app generate_images)

# Include generated directory
target_include_directories(app PRIVATE ${GEN_DIR})

target_sources(app PRIVATE
    src/main.c
    ${GEN_C}
)
