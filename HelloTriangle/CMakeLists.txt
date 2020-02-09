set(TARGET_NAME HelloTriangle)
add_executable(${TARGET_NAME} WIN32
    Main.cpp
    Scene.cpp
    D3D12HelloTriangle.cpp
    CommandBuilder.cpp
    Fence.cpp
    Swapchain.cpp
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    UNICODE
    _UNICODE
    )
target_include_directories(${TARGET_NAME} PRIVATE
    )   
target_link_directories(${TARGET_NAME} PRIVATE
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    d3d12
    dxgi
    d3dcompiler
    )

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
