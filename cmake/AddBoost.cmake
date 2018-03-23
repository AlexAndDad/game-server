macro(AddBoost AddBoost_TARGET)
    set(AddBoost_options)
    set(AddBoost_oneValueArgs)
    set(AddBoost_multiValueArgs COMPONENTS)
    cmake_parse_arguments(AddBoost "${AddBoost_options}" "${AddBoost_oneValueArgs}"
            "${AddBoost_multiValueArgs}" ${ARGN})

    set(AddBoost_libs Boost::boost)

    if (AddBoost_COMPONENTS)
        hunter_add_package(Boost COMPONENTS ${AddBoost_COMPONENTS})
        find_package(Boost REQUIRED CONFIG COMPONENTS ${AddBoost_COMPONENTS})
    else ()
        hunter_add_package(Boost)
        find_package(Boost REQUIRED CONFIG)
    endif ()

    foreach (comp ${AddBoost_COMPONENTS})
        list(APPEND AddBoost_Libs "Boost::${comp}")
    endforeach ()

    target_link_libraries(${AddBoost_TARGET} PUBLIC ${AddBoost_Libs})

endmacro()
