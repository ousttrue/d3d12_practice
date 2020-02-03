cmake_minimum_required(VERSION 3.0.0)
project(hello12)

add_executable(${PROJECT_NAME}
    main.cpp
    lodepng.cpp
    Matrices.cpp
    pathtools.cpp
    strtools.cpp
    )
target_include_directories(${PROJECT_NAME} PRIVATE
    $ENV{VCPKG_DIR}/installed/x64-windows/include/SDL2
    $ENV{VCPKG_DIR}/installed/x64-windows/include
    )   
target_link_directories(${PROJECT_NAME} PRIVATE
    $ENV{VCPKG_DIR}/installed/x64-windows/lib
    )
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12
    dxgi
    d3dcompiler
    SDL2
    openvr_api

    optimized $ENV{VCPKG_DIR}/installed/x64-windows/lib/manual-link/SDL2Main.lib
    debug $ENV{VCPKG_DIR}/installed/x64-windows/debug/lib/manual-link/SDL2Maind.lib
    )