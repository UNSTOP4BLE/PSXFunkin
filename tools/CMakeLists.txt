cmake_minimum_required(VERSION 3.20)

project(
	psxfunkin-tools
	LANGUAGES C CXX
	VERSION 1.0.0
	DESCRIPTION "Tools used to build psxfunkin"
)

# funkinarcpak
add_executable(funkinarcpak funkinarcpak/funkinarcpak.c)

# funkinchartpak
add_executable(funkinchartpak funkinchartpak/funkinchartpak.cpp)

# funkinmultitool
add_executable(funkinmultitool funkinmultitool/funkinmultitool.cpp)

# funkintimconv
add_executable(funkintimconv funkintimconv/funkintimconv.c)

# funkinmuspak
add_executable(funkinmuspak 
	funkinmuspak/funkinmuspak.cpp
	funkinmuspak/adpcm.cpp
)