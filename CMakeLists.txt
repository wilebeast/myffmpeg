cmake_minimum_required(VERSION 3.17)
project(myffmpeg)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -v -stdlib=libc++ -framework OpenGL -framework AppKit -framework Security -framework CoreFoundation -framework CoreVideo -framework CoreMedia -framework QuartzCore -framework CoreFoundation -framework VideoDecodeAcceleration -framework Cocoa -framework AudioToolbox -framework VideoToolbox -framework OpenCL")

set(SOURCE_FILES extract.c)
set(INC_DIR /usr/local/include/)
set(LINK_DIR /usr/local/lib/ /lib)
include_directories(.)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})

add_executable(myffmpeg
        extract.c)
add_definitions("-D__STDC_CONSTANT_MACROS")
target_link_libraries(
        myffmpeg
        iconv
        bz2
        z
        lzma
        avfilter
        avutil
        avdevice
        avformat
        swscale
        avcodec
        swresample
)
