set(TARGET_NAME hello_imgui)
add_executable(${TARGET_NAME}
    main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/examples/imgui_impl_win32.cpp
    ${IMGUI_DIR}/examples/imgui_impl_dx12.cpp
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/examples
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    d3d12
    dxgi
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    DEBUG
    DX12_ENABLE_DEBUG_LAYER
    )
