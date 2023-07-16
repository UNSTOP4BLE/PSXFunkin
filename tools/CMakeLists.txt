cmake_minimum_required(VERSION 3.20)

project(
    psxfunkin-tools
    LANGUAGES C CXX
    VERSION 1.0.0
    DESCRIPTION "Tools used to build psxfunkin"
)
set(CMAKE_C_STANDARD           11)
set(CMAKE_CXX_STANDARD         17)

# funkinarcpak
add_executable(funkinarcpak funkinarcpak/funkinarcpak.c)

# funkinchartpak
add_executable(funkinchartpak funkinchartpak/funkinchartpak.cpp)

# funkinmultitool
add_executable(funkinmultitool funkinmultitool/funkinmultitool.cpp)

# funkintimconv
add_executable(funkintimconv funkintimconv/funkintimconv.c)

# funkinchrpak
add_executable(funkinchrpak funkinchrpak/funkinchrpak.cpp)

# funkinvagpak
add_executable(funkinvagpak 
   funkinvagpak/funkinvagpak.cpp
   funkinvagpak/adpcm.cpp
)
find_package(Threads REQUIRED)
target_link_libraries(funkinvagpak Threads::Threads)


# psxavenc
find_path(_avcodec_include libavcodec/avcodec.h REQUIRED)
find_path(_avformat_include libavformat/avformat.h REQUIRED)
find_path(_avutil_include libavutil/avutil.h REQUIRED)
find_path(_swresample_include libswresample/swresample.h REQUIRED)
find_path(_swscale_include libswscale/swscale.h REQUIRED)
find_library(_avcodec avcodec REQUIRED)
find_library(_avformat avformat REQUIRED)
find_library(_avutil avutil REQUIRED)
find_library(_swresample swresample REQUIRED)
find_library(_swscale swscale REQUIRED)

# libav*
add_library(libav INTERFACE)
target_include_directories(libav INTERFACE
    ${_avcodec_include}
    ${_avformat_include}
    ${_avutil_include}
    ${_swresample_include}
    ${_swscale_include}
)
target_link_libraries(libav INTERFACE
    ${_avcodec}
    ${_avformat}
    ${_avutil}
    ${_swresample}
    ${_swscale}
)

# psxavenc
add_executable(psxavenc 
    psxavenc/psxavenc.c 
    psxavenc/mdec.c 
    psxavenc/filefmt.c 
    psxavenc/decoding.c 
    psxavenc/cdrom.c 
    psxavenc/libpsxav/adpcm.c 
    psxavenc/libpsxav/cdrom.c
)
target_link_libraries(psxavenc PRIVATE libav)