# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.20)

# define the project
project(
	funkin
	LANGUAGES    C
	VERSION      1.0.0
	DESCRIPTION  "PSXFunkin on PSn00bSDK"
	HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
)

# build charts
file(GLOB _json_charts iso/charts/*.json)
list(TRANSFORM _json_charts APPEND .cht OUTPUT_VARIABLE _charts)
foreach(_file IN LISTS _json_charts)
    add_custom_command(
        OUTPUT ${_file}.cht
        COMMAND ./tools/build/funkinchartpak ${_file}
        DEPENDS ${_file}
        COMMENT "Building chart ${_file}"
    )
endforeach()

# compile the source code
set(_sources
    src/main.c 
    src/mutil.c 
    src/random.c 
    src/archive.c 
    src/font.c 
    src/trans.c 
    src/loadscr.c 
    src/menu.c 
    src/stage.c 
    src/debug.c 
    src/save.c 
    src/psx/psx.c 
    src/psx/io.c 
    src/psx/gfx.c 
    src/psx/audio.c 
    src/psx/pad.c 
    src/psx/timer.c 
    src/stage/dummy.c 
    src/stage/week1.c 
    src/stage/week2.c
    src/stage/week3.c 
    src/stage/week4.c 
    src/stage/week5.c 
    src/stage/week6.c 
    src/animation.c 
    src/character.c 
    src/character/bf.c 
    src/character/bfweeb.c 
    src/character/speaker.c 
    src/character/dad.c 
    src/character/spook.c 
    src/character/pico.c 
    src/character/mom.c 
    src/character/xmasp.c 
    src/character/monster.c 
    src/character/senpai.c 
    src/character/senpaim.c 
    src/character/spirit.c 
    src/character/gf.c 
    src/character/gfweeb.c 
    src/object.c 
    src/object/combo.c 
    src/object/splash.c 
    src/pause.c 
)

psn00bsdk_add_executable(funkin STATIC ${_sources})

install(FILES ${PROJECT_BINARY_DIR}/funkin.exe TYPE BIN)

# create bin and cue
psn00bsdk_add_cd_image(iso
    psxfunkin # file name
    funkin.xml # path to xml
    DEPENDS funkin ${_charts}
)