# Android-Hide-Import
Small &amp; Easy helper for hiding imports in ELF binaries

## Building with the Android NDK

To compile the sample code with the Android NDK you can use CMake. Make sure `ANDROID_NDK_HOME` points to your NDK install and run the following commands:

```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21
cmake --build .
```

The `CMakeLists.txt` below builds `Main.cpp` together with the sources under `xdl/`:

```cmake
cmake_minimum_required(VERSION 3.6)
project(AndroidHideImport LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(xdl STATIC
    xdl/xdl.c
    xdl/xdl_iterate.c
    xdl/xdl_linker.c
    xdl/xdl_lzma.c
    xdl/xdl_util.c
)

target_include_directories(xdl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/xdl/include
)

add_executable(sample
    Main.cpp
)

target_link_libraries(sample PRIVATE xdl dl)
```

## IDA PRO 9.0 output example:
![image](https://github.com/user-attachments/assets/e604253a-8a40-40ba-99e7-1448e165ae2c)
