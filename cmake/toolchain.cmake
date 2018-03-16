
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/polly/cxx17.cmake)

    if (NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
        execute_process(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND git submodule init)
        execute_process(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND git submodule update polly)
    endif ()

    message(STATUS "[project] CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
endif ()

