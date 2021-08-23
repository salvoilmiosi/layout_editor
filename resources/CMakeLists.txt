set(editor_resources
icon_editor.png
tool_select.png
tool_newbox.png
tool_deletebox.png
tool_resize.png
tool_test.png
tool_move_page.png
tool_reload.png
tool_abort.png
tool_rotate.png
tool_load_pdf.png
tool_find_layout.png
tool_settings.png
)

foreach(file ${editor_resources})
    get_filename_component(varname "${file}" NAME)
    string(MAKE_C_IDENTIFIER "${varname}" varname)

    set(resource_c_file "${CMAKE_CURRENT_BINARY_DIR}/${file}.c")
    list(APPEND resource_c_files "${resource_c_file}")

    add_custom_command(
        OUTPUT "${resource_c_file}"
        COMMAND ${CMAKE_COMMAND}
            -D "resource_file_name=${CMAKE_CURRENT_SOURCE_DIR}/${file}"
            -D "source_file_name=${resource_c_file}"
            -D "variable_name=__resource__${varname}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/EmbedResource.cmake"
        DEPENDS "${file}"
        VERBATIM
    )
endforeach()

add_library(resources OBJECT ${resource_c_files})