# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.20)

# define the project
project(
	funkin
	LANGUAGES    C ASM
	VERSION      1.0.0
	DESCRIPTION  "PSXFunkin on PSn00bSDK"
	HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
)

# find tools
find_program(_chartpak
	funkinchartpak REQUIRED
	HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_arcpak
	funkinarcpak REQUIRED
	HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_timconv
	funkintimconv REQUIRED
	HINTS ${PROJECT_SOURCE_DIR}/tools/build
)

# custom functions

function(funkin_add_archive _out)
	add_custom_command(
		OUTPUT ${_out}
		COMMAND ${_arcpak} ${_out} ${ARGN}
		DEPENDS ${ARGN}
		COMMENT "Building archive ${_out}"
	)
endfunction()

# build charts
file(
	GLOB_RECURSE _charts
	RELATIVE ${PROJECT_SOURCE_DIR}
	iso/chart/*.json
)
list(
	TRANSFORM _charts REPLACE "\.json$" ".cht"
	OUTPUT_VARIABLE _built_charts
)

foreach(_in _out IN ZIP_LISTS _charts _built_charts)
	add_custom_command(
		OUTPUT ${_out}
		COMMAND ${_chartpak} ${_out} ${PROJECT_SOURCE_DIR}/${_in}
		DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
		COMMENT "Building chart ${_out}"
	)
endforeach()

# build .tim images
file(
	GLOB_RECURSE _images
	RELATIVE ${PROJECT_SOURCE_DIR}
	iso/*.png
)
list(
	TRANSFORM _images REPLACE "\.png$" ".tim"
	OUTPUT_VARIABLE _built_images
)

foreach(_in _out IN ZIP_LISTS _images _built_images)
	add_custom_command(
		OUTPUT ${_out}
		COMMAND ${_timconv} ${_out} ${PROJECT_SOURCE_DIR}/${_in}
		DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
		COMMENT "Building image ${_out}"
	)
endforeach()

# build .arc files

# menu
funkin_add_archive(iso/menu/menu.arc
	iso/menu/back.tim
	iso/menu/ng.tim
	iso/menu/story.tim
	iso/menu/title.tim
)

# bf
funkin_add_archive(iso/bf/main.arc
    iso/bf/bf0.tim 
    iso/bf/bf1.tim 
    iso/bf/bf2.tim 
    iso/bf/bf3.tim 
    iso/bf/bf4.tim 
    iso/bf/bf5.tim 
    iso/bf/bf6.tim 
    iso/bf/dead0.tim
)
funkin_add_archive(iso/bf/dead.arc
    iso/bf/dead1.tim 
    iso/bf/dead2.tim 
    iso/bf/retry.tim
)
funkin_add_archive(iso/bf/weeb.arc
    iso/bf/weeb0.tim 
    iso/bf/weeb1.tim
)
funkin_add_archive(iso/bf/xmas.arc
    iso/bf/xmasbf0.tim 
    iso/bf/xmasbf1.tim 
    iso/bf/xmasbf2.tim 
    iso/bf/xmasbf3.tim 
    iso/bf/xmasbf4.tim 
    iso/bf/xmasbf5.tim
    iso/bf/dead0.tim
)

# dad
funkin_add_archive(iso/dad/main.arc
    iso/dad/idle0.tim 
    iso/dad/idle1.tim 
    iso/dad/left.tim 
    iso/dad/down.tim 
    iso/dad/up.tim 
    iso/dad/right.tim
)

# spook
funkin_add_archive(iso/spook/main.arc
    iso/spook/idle0.tim 
    iso/spook/idle1.tim 
    iso/spook/idle2.tim 
    iso/spook/left.tim 
    iso/spook/down.tim 
    iso/spook/up.tim 
    iso/spook/right.tim
)

# pico
funkin_add_archive(iso/pico/main.arc
	iso/pico/idle.tim
	iso/pico/hit0.tim 
	iso/pico/hit1.tim
)

# mom
funkin_add_archive(iso/mom/main.arc
	iso/mom/idle0.tim 
	iso/mom/idle1.tim 
	iso/mom/left.tim 
	iso/mom/down.tim 
	iso/mom/up.tim 
	iso/mom/right.tim
)

# xmas parents
funkin_add_archive(iso/xmasp/main.arc
	iso/xmasp/idle0.tim 
	iso/xmasp/idle1.tim 
	iso/xmasp/idle2.tim 
	iso/xmasp/idle3.tim 
	iso/xmasp/lefta0.tim 
	iso/xmasp/lefta1.tim 
	iso/xmasp/leftb0.tim 
	iso/xmasp/leftb1.tim 
	iso/xmasp/downa0.tim 
	iso/xmasp/downa1.tim 
	iso/xmasp/downb0.tim
	iso/xmasp/downb1.tim 
	iso/xmasp/upa0.tim 
	iso/xmasp/upa1.tim 
	iso/xmasp/upb0.tim 
	iso/xmasp/upb1.tim 
	iso/xmasp/righta0.tim 
	iso/xmasp/righta1.tim 
	iso/xmasp/rightb0.tim 
	iso/xmasp/rightb1.tim
)

# monster
funkin_add_archive(iso/monster/main.arc
	iso/monster/idle0.tim 
	iso/monster/idle1.tim 
	iso/monster/left.tim 
	iso/monster/down.tim 
	iso/monster/up.tim
	iso/monster/right.tim
)

funkin_add_archive(iso/monster/xmas.arc
	iso/monster/idleb0.tim 
	iso/monster/idleb1.tim 
	iso/monster/leftb.tim 
	iso/monster/downb.tim 
	iso/monster/upb.tim 
	iso/monster/rightb.tim
)

# senpai
funkin_add_archive(iso/senpai/main.arc
	iso/senpai/senpai0.tim 
	iso/senpai/senpai1.tim
)

funkin_add_archive(iso/senpaim/main.arc
	iso/senpaim/senpai0.tim
	iso/senpaim/senpai1.tim
)

# Spirit
iso/spirit/main.arc: iso/spirit/spirit0.tim iso/spirit/spirit1.tim

# GF
iso/gf/main.arc: iso/gf/gf0.tim iso/gf/gf1.tim iso/gf/gf2.tim
iso/gf/xmas.arc: iso/gf/xmasgf0.tim iso/gf/xmasgf1.tim iso/gf/xmasgf2.tim
iso/gf/tut.arc: iso/gf/tut0.tim iso/gf/tut1.tim
iso/gf/weeb.arc: iso/gf/weeb0.tim iso/gf/weeb1.tim

# Week 1
iso/week1/back.arc: iso/week1/back0.tim iso/week1/back1.tim

# Week 2
iso/week2/back.arc: iso/week2/back0.tim iso/week2/back1.tim iso/week2/back2.tim

# Week 3
iso/week3/back.arc: iso/week3/back0.tim iso/week3/back1.tim iso/week3/back2.tim iso/week3/back3.tim iso/week3/back4.tim 

# Week 4
iso/week4/back.arc: iso/week4/back0.tim iso/week4/back1.tim iso/week4/back2.tim iso/week4/back3.tim
iso/week4/hench.arc: iso/week4/hench0.tim iso/week4/hench1.tim

# Week 5
iso/week5/back.arc: iso/week5/back0.tim iso/week5/back1.tim iso/week5/back2.tim iso/week5/back3.tim iso/week5/back4.tim iso/week5/back5.tim iso/week5/back6.tim 

# Week6
iso/week6/back.arc: iso/week6/back0.tim iso/week6/back1.tim

# Week 7
iso/week7/back.arc: iso/week7/back0.tim iso/week7/back1.tim iso/week7/back2.tim iso/week7/back3.tim

# ADD OTHER ARCHIVES HERE

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

# create bin and cue
psn00bsdk_add_cd_image(iso
	funkin # file name
	funkin.xml # path to xml
	DEPENDS funkin iso/menu/menu.arc ${_built_charts} ${_built_images}
)
