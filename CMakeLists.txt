cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uwb_scanner VERSION 1.0.0)

# Get git commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Get build timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

# Pass version info as compile definitions
if(GIT_COMMIT_HASH)
    target_compile_definitions(app PRIVATE
        GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
        BUILD_TIMESTAMP="${BUILD_TIMESTAMP}"
    )
endif()

target_sources(app PRIVATE
    src/main.c
    src/uwb_scanner.c
    src/dw3000_driver.c
    src/uart_output.c
)

target_include_directories(app PRIVATE
    include
)
