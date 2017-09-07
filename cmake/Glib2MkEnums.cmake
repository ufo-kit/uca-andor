find_program(GLIB2_MKENUMS glib-mkenums REQUIRED)

function(glib2_mkenums prefix template_prefix header_list)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.h
        COMMAND ${GLIB2_MKENUMS}
        ARGS
            --template ${template_prefix}.h.template
            ${header_list} > ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.h
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${header_list})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.c
        COMMAND ${GLIB2_MKENUMS}
        ARGS
            --template ${template_prefix}.c.template
            ${header_list} > ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.c
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${header_list}
                ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.h
        )
endfunction()
