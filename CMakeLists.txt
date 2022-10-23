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
find_program(_psxavenc
	psxavenc REQUIRED
	HINTS ${PROJECT_SOURCE_DIR}/tools/build
)
find_program(_xainterleave
	xainterleave REQUIRED
	HINTS ${PROJECT_SOURCE_DIR}/tools/build
)

# custom functions
set(_built_archives "")
set(_built_xa "")

function(funkin_add_archive _out)
	set(_built_archives ${_built_archives} ${_out} PARENT_SCOPE)

	add_custom_command(
		OUTPUT ${_out}
		COMMAND ${_arcpak} ${_out} ${ARGN}
		DEPENDS ${ARGN}
		COMMENT "Building archive ${_out}"
	)
endfunction()

function(funkin_interleave _out _in)
	set(_built_xa ${_built_xa} ${_out} PARENT_SCOPE)

	# this actually copies the manifest to the build directory
	configure_file(${_in} ${_in} NEWLINE_STYLE LF)
	add_custom_command(
		OUTPUT ${_out}
		COMMAND ${_xainterleave} ${_out} ${_in}
		DEPENDS ${_in} ${ARGN}
		COMMENT "Interleaving xa ${_out}"
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

# build .xa files
file(
	GLOB_RECURSE _xa
	RELATIVE ${PROJECT_SOURCE_DIR}
	iso/*.ogg
)
list(
	TRANSFORM _xa REPLACE "\.ogg$" ".xa"
	OUTPUT_VARIABLE _built_audio
)

foreach(_in _out IN ZIP_LISTS _xa _built_audio)
	add_custom_command(
		OUTPUT ${_out}
		COMMAND ${_psxavenc} -f 37800 -t xa -b 4 -c 2 -F 1 -C 0 ${PROJECT_SOURCE_DIR}/${_in} ${_out} 
		DEPENDS ${PROJECT_SOURCE_DIR}/${_in}
		COMMENT "Building xa ${_out}"
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

# gf
funkin_add_archive(iso/gf/main.arc
    iso/gf/gf0.tim 
    iso/gf/gf1.tim 
    iso/gf/gf2.tim
)
funkin_add_archive(iso/gf/xmas.arc
    iso/gf/xmasgf0.tim 
    iso/gf/xmasgf1.tim 
    iso/gf/xmasgf2.tim
)
funkin_add_archive(iso/gf/tut.arc
    iso/gf/tut0.tim
    iso/gf/tut1.tim
)
funkin_add_archive(iso/gf/weeb.arc
    iso/gf/weeb0.tim
    iso/gf/weeb1.tim
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

# senpai
funkin_add_archive(iso/senpai/main.arc
	iso/senpai/senpai0.tim 
	iso/senpai/senpai1.tim
)

funkin_add_archive(iso/senpaim/main.arc
	iso/senpaim/senpai0.tim
	iso/senpaim/senpai1.tim
)

# spirit
funkin_add_archive(iso/spirit/main.arc
	iso/spirit/spirit0.tim
	iso/spirit/spirit1.tim
)

# week 1
funkin_add_archive(iso/week1/back.arc
	iso/week1/back0.tim
	iso/week1/back1.tim
)

# week 2
funkin_add_archive(iso/week2/back.arc
	iso/week2/back0.tim
	iso/week2/back1.tim
	iso/week2/back2.tim
)

# week 3
funkin_add_archive(iso/week3/back.arc
	iso/week3/back0.tim
	iso/week3/back1.tim 
	iso/week3/back2.tim 
	iso/week3/back3.tim 
	iso/week3/back4.tim 
)

# week 4
funkin_add_archive(iso/week4/back.arc
	iso/week4/back0.tim 
	iso/week4/back1.tim 
	iso/week4/back2.tim 
	iso/week4/back3.tim
)
funkin_add_archive(iso/week4/hench.arc
	iso/week4/hench0.tim 
	iso/week4/hench1.tim
)

# week 5
funkin_add_archive(iso/week5/back.arc
	iso/week5/back0.tim 
	iso/week5/back1.tim 
	iso/week5/back2.tim 
	iso/week5/back3.tim 
	iso/week5/back4.tim
	iso/week5/back5.tim 
	iso/week5/back6.tim 
)

# week6
funkin_add_archive(iso/week6/back.arc
	iso/week6/back0.tim
	iso/week6/back1.tim
) 

# ADD ARCHIVES HERE

# interleave xa
funkin_interleave(iso/music/menu.xa iso/music/menu.txt
	iso/music/freaky.xa
	iso/music/gameover.xa
)
funkin_interleave(iso/music/week1a.xa iso/music/week1a.txt
	iso/music/bopeebo/Inst.xa
	iso/music/bopeebo/Vocals.xa
	iso/music/fresh/Inst.xa
	iso/music/fresh/Vocals.xa
)
funkin_interleave(iso/music/week1b.xa iso/music/week1b.txt
	iso/music/dadbattle/Inst.xa
	iso/music/dadbattle/Vocals.xa
	iso/music/tutorial/Inst.xa
)
funkin_interleave(iso/music/week2a.xa iso/music/week2a.txt
	iso/music/spookeez/Inst.xa
	iso/music/spookeez/Vocals.xa
	iso/music/south/Inst.xa
	iso/music/south/Vocals.xa
)
funkin_interleave(iso/music/week2b.xa iso/music/week2b.txt
	iso/music/monster/Inst.xa
	iso/music/monster/Vocals.xa
)
funkin_interleave(iso/music/week3a.xa iso/music/week3a.txt
	iso/music/pico/Inst.xa
	iso/music/pico/Vocals.xa
	iso/music/philly/Inst.xa
	iso/music/philly/Vocals.xa
)
funkin_interleave(iso/music/week3b.xa iso/music/week3b.txt
	iso/music/blammed/Inst.xa
	iso/music/blammed/Vocals.xa
)
funkin_interleave(iso/music/week4a.xa iso/music/week4a.txt
	iso/music/satin-panties/Inst.xa
	iso/music/satin-panties/Vocals.xa
	iso/music/high/Inst.xa
	iso/music/high/Vocals.xa
)
funkin_interleave(iso/music/week4b.xa iso/music/week4b.txt
	iso/music/milf/Inst.xa
	iso/music/milf/Vocals.xa
	iso/music/test/Inst.xa
	iso/music/test/Vocals.xa
)
funkin_interleave(iso/music/week5a.xa iso/music/week5a.txt
	iso/music/cocoa/Inst.xa
	iso/music/cocoa/Vocals.xa
	iso/music/eggnog/Inst.xa
	iso/music/eggnog/Vocals.xa
)
funkin_interleave(iso/music/week5b.xa iso/music/week5b.txt
	iso/music/winter-horrorland/Inst.xa
	iso/music/winter-horrorland/Vocals.xa
)
funkin_interleave(iso/music/week6a.xa iso/music/week6a.txt
	iso/music/senpai/Inst.xa
	iso/music/senpai/Vocals.xa
	iso/music/roses/Inst.xa
	iso/music/roses/Vocals.xa
)
funkin_interleave(iso/music/week6b.xa iso/music/week6b.txt
	iso/music/thorns/Inst.xa
	iso/music/thorns/Vocals.xa
)

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
	DEPENDS 
	funkin   
	${_built_images}
	${_built_charts} 
	${_built_audio} 
	${_built_archives}
	${_built_xa}
)
