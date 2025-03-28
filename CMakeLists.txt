cmake_minimum_required(VERSION 3.3)

project(OCR)

set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(OUT_PATH ${CMAKE_BINARY_DIR})
set(NDK_PATH /opencv4/install/android-ndk-r28)


MESSAGE(STATUS "This is ROOT_PATH dir " ${ROOT_PATH})
MESSAGE(STATUS "This is OUT_PATH dir " ${OUT_PATH})
MESSAGE(STATUS "This is NDK_PATH dir " ${NDK_PATH})
MESSAGE(STATUS "This is OPENCV_PATH dir " ${OPENCV_PATH})
#嵌入调试符号 以便gdb调试
add_definitions("-Wall -g")





set(CMAKE_SYSTEM_NAME Android)            #构建成android platform 
set(CMAKE_SYSTEM_PROCESSOR arm)           #构建目标架构
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)   #编译二进制接口（armeabi-v7a、arm64-v8a、x86、x86_64）cmake --help-variable-list
# Cmake预定义变量 可自动查看andriod 模块的头文件和库库文件,不需要额外使用include_directories 
set(CMAKE_ANDROID_NDK ${NDK_PATH})   #andorid ndk


set(triple aarch64-linux-android)
set(CMAKE_ANDROID_TOOLCHAIN ${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang++)
set(CMAKE_CXX_COMPILER ${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_C_COMPILER ${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ANDROID_PLATFORM android-33)
set(CMAKE_BUILD_TYPE Release)
# 编译时库 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fPIC -frtti")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 链接运行时库
set(CMAKE_ANDROID_STL_TYPE c++_shared)


include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
    message(STATUS "Compiler supports C++17")
else()
    message(STATUS "Compiler does not support C++17")
endif()

#大概意思可能是选用NDK 里面的cmake，我试过不加好像也可以
#set(CMAKE_TOOLCHAIN_FILE ${NDK_PATH}/build/cmake/android.toolchain.cmake)
#set(OPENCV_PATH ${ROOT_PATH}/lib/opencv-4.9.0)


set(OpenCVSDK /opencv4/OpenCV-android-sdk/sdk/native)



#查找 CMake 配置文件
set(CMAKE_PREFIX_PATH ${OpenCVSDK}/jni/abi-arm64-v8a)
#默认情况下，CMake 会在以下目录查找这些配置文件：
#1.系统的 CMake 模块路径（通常在 /usr/share/cmake/Modules 或类似路径）
#2.环境变量 CMAKE_PREFIX_PATH 中指定的路径
find_package(OpenCV REQUIRED)

#查找头文件
include_directories(
	${OpenCVSDK}/jni/include
    ${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/android
	)

link_directories(
    ${OpenCVSDK}/staticlibs/arm64-v8a
    ${OpenCVSDK}/3rdparty/libs/arm64-v8a
    ${OpenCVSDK}/libs/arm64-v8a   
    )


link_libraries(
        log
		opencv_imgproc
		opencv_imgcodecs
		opencv_core
    )


# add_executable(treasure 
#     main_.cpp
# 	)


# set_target_properties(treasure PROPERTIES
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "$ORIGIN/libs"
# )
# target_include_directories(treasure PRIVATE ${ROOT_PATH}/include)
# target_link_directories(treasure PRIVATE ${ROOT_PATH}/libs)
# target_link_libraries(treasure PRIVATE SDL3)
# # target_link_libraries(treasure PRIVATE c++_static)
# target_link_libraries(treasure ${link_libraries})

# add_executable(shark 
#     shark.cpp
# 	)


# set_target_properties(shark PROPERTIES
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "$ORIGIN/libs"
# )
# # add_library(OpenCV SHARED IMPORTED)
# # set_target_properties(OpenCV PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/libopencv.so)
# # target_link_libraries(shark PRIVATE OpenCV)

# target_include_directories(shark PRIVATE ${ROOT_PATH}/include)
# target_link_directories(shark PRIVATE ${ROOT_PATH}/libs)
# target_link_libraries(shark PRIVATE SDL3)
# target_link_libraries(shark ${link_libraries})



add_executable(dummy 
    dummy.cpp
	)


set_target_properties(dummy PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN/libs"
)
# set_target_properties(dummy PROPERTIES
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "$ORIGIN/libs"
# )
# add_library(OpenCV SHARED IMPORTED)
# set_target_properties(OpenGL PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/libopencv.so)
# target_link_libraries(gold PRIVATE OpenGL)

target_include_directories(dummy PRIVATE ${ROOT_PATH}/include/drm)
target_include_directories(dummy PRIVATE ${ROOT_PATH}/include)
target_link_directories(dummy PRIVATE ${ROOT_PATH}/libs)
target_link_libraries(dummy PRIVATE SDL3)
target_link_libraries(dummy PRIVATE EGL)
target_link_libraries(dummy PRIVATE GLESv2)
target_link_libraries(dummy PRIVATE drm)
# target_link_libraries(gold PRIVATE c++_static)
target_link_libraries(dummy ${link_libraries})


# add_executable(beach 
#     beach.cpp
# 	)

# set_target_properties(beach PROPERTIES
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "$ORIGIN/libs"
# )
# target_include_directories(beach PRIVATE ${ROOT_PATH}/include)
# target_link_directories(beach PRIVATE ${ROOT_PATH}/libs)
# target_link_libraries(beach PRIVATE SDL3)
# target_link_libraries(beach PRIVATE c++_static)
# target_link_libraries(beach ${link_libraries})

