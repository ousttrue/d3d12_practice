set(TARGET_NAME hellovr_dx12)
add_executable(${TARGET_NAME}
    CMainApplication.cpp
    DX12RenderModel.cpp
    SDLApplication.cpp
    Commandline.cpp
    Hmd.cpp
    DeviceRTV.cpp
    dprintf.cpp
    main.cpp
    lodepng.cpp
    Matrices.cpp
    pathtools.cpp
    strtools.cpp
    )
target_include_directories(${TARGET_NAME} PRIVATE
    $ENV{VCPKG_DIR}/installed/x64-windows/include/SDL2
    $ENV{VCPKG_DIR}/installed/x64-windows/include
    )   
target_link_directories(${TARGET_NAME} PRIVATE
    $ENV{VCPKG_DIR}/installed/x64-windows/lib
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    d3d12
    dxgi
    d3dcompiler
    SDL2
    openvr_api

    optimized $ENV{VCPKG_DIR}/installed/x64-windows/lib/manual-link/SDL2Main.lib
    debug $ENV{VCPKG_DIR}/installed/x64-windows/debug/lib/manual-link/SDL2Maind.lib
    )
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)