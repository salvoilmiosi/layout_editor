cmake_minimum_required(VERSION 3.0.0)
project(layout_editor VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

set(editor_SRCS
src/box_dialog.cpp
src/box_editor_panel.cpp
src/clipboard.cpp
src/editor.cpp
src/editor_evt.cpp
src/image_panel.cpp
src/layout_options_dialog.cpp
src/main.cpp
src/output_dialog.cpp
src/pdf_to_image.cpp
)

set(editor_RESOURCES
resources/icon_editor.png
resources/tool_select.png
resources/tool_newbox.png
resources/tool_deletebox.png
resources/tool_resize.png
resources/tool_test.png
resources/tool_reload.png
resources/tool_abort.png
)

function(embed_resource resource_file_name source_file_name variable_name)

    file(READ ${resource_file_name} hex_content HEX)
    file(SIZE ${resource_file_name} content_size)

    string(REPEAT "[0-9a-f]" 32 column_pattern)
    string(REGEX REPLACE "(${column_pattern})" "\\1\n" content "${hex_content}")

    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")

    string(REGEX REPLACE ", \n?$" "" content "${content}")

    set(array_definition "const char ${variable_name}[${content_size}] = {\n${content}\n};")
    set(length_definition "const int ${variable_name}_length = ${content_size};")

    set(source "// Auto generated file.\n${array_definition}\n${length_definition}\n")

    file(WRITE "${source_file_name}" "${source}")

endfunction()

foreach(file ${editor_RESOURCES})
    get_filename_component(basename "${file}" NAME)
    string(MAKE_C_IDENTIFIER basename "${basename}")
    string(REPLACE "." "_" basename "${basename}")

    set(resource_file_c "${CMAKE_CURRENT_BINARY_DIR}/${file}.c")
    list(APPEND editor_RESOURCES_C "${resource_file_c}")

    embed_resource("${CMAKE_CURRENT_SOURCE_DIR}/${file}" "${resource_file_c}" "__resource__${basename}")
endforeach()

find_package(wxWidgets REQUIRED COMPONENTS core stc)
include(${wxWidgets_USE_FILE})

set(editor_ALL_SOURCES ${editor_SRCS} ${editor_RESOURCES_C} resources/resources.rc)
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND WIN32)
    add_executable(editor WIN32 ${editor_ALL_SOURCES})
else()
    add_executable(editor ${editor_ALL_SOURCES})
endif()
target_link_libraries(editor ${wxWidgets_LIBRARIES} bls)

install(TARGETS editor)