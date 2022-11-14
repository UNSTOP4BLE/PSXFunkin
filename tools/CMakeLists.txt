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

# funkinvagpak
add_executable(funkinvagpak 
	funkinvagpak/funkinvagpak.cpp
	funkinvagpak/adpcm.cpp
)
find_package(Threads REQUIRED)
target_link_libraries(funkinvagpak Threads::Threads)