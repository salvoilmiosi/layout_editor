cmake_minimum_required(VERSION 3.0.0)
project(layout_editor VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(editor_sources
src/box_dialog.cpp
src/box_editor_panel.cpp
src/clipboard.cpp
src/editor.cpp
src/editor_evt.cpp
src/image_panel.cpp
src/layout_options_dialog.cpp
src/main.cpp
src/move_page_dialog.cpp
src/output_dialog.cpp
src/page_ctl.cpp
resources/resources.rc
)

find_package(wxWidgets REQUIRED COMPONENTS base core stc)
include(${wxWidgets_USE_FILE})

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND WIN32)
    add_executable(blseditor WIN32 ${editor_sources})
else()
    add_executable(blseditor ${editor_sources})
endif()
target_link_libraries(blseditor PRIVATE ${wxWidgets_LIBRARIES} bls::bls)

add_subdirectory(resources)
target_link_libraries(blseditor PRIVATE resources)

install(TARGETS blseditor)