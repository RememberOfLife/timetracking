cmake_minimum_required(VERSION 3.18)
set (CMAKE_C_STANDARD 11)

project(timetracking C)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(NOT CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg].*")
    if(supported)
        message(STATUS "IPO/LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: <${error}>")
    endif()
else()
    message(STATUS "IPO/LTO disabled in non-release build")
endif()

set(INCLUDES
    lib
    src
)

set(SOURCES
    src/main.c
)

add_executable(timetracking "${SOURCES}")

target_compile_options(timetracking PRIVATE
    # "-Wall"
    # "-Wextra"
    # "-Werror" # warnings as errors
    "-Wfatal-errors" # stop after first error
)

target_include_directories(timetracking PRIVATE ${INCLUDES})

set_target_properties(timetracking PROPERTIES EXPORT_COMPILE_COMMANDS true)
