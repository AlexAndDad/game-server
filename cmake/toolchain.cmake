

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)

    if (NOT DEFINED POLLY_ROOT)
        set(POLLY_ROOT ${CMAKE_SOURCE_DIR}/polly)
    endif ()

    message(STATUS "[project] POLLY_ROOT: ${POLLY_ROOT}")


    if (NOT EXISTS "${POLLY_ROOT}")
        execute_process(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND git submodule add git@github.com:AlexAndDad/polly.git)
    endif ()

    set(CMAKE_TOOLCHAIN_FILE ${POLLY_ROOT}/cxx17.cmake)

    if (NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
        execute_process(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND git submodule init)
        execute_process(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND git submodule update polly)
    endif ()

endif ()

message(STATUS "[project] CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
include(${CMAKE_TOOLCHAIN_FILE})
