set(TARGET_NAME HelloConstBuffers)
add_executable(${TARGET_NAME} WIN32
    Main.cpp
    D3D12HelloConstBuffers.cpp
    CD3D12Scene.cpp
    CD3D12SwapChain.cpp
    ResourceItem.cpp
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
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
