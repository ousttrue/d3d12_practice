cmake_minimum_required(VERSION 3.0.0)
project(hello12)
set(IMGUI_DIR ${CMAKE_CURRENT_LIST_DIR}/external/imgui)
subdirs(hellovr_dx12 HelloTriangle hello_imgui)
