#---------------------------------------------------------------------------------------------------------------------
# TARGET is the name of the output.
# BUILD is the directory where object files & intermediate files will be placed.
# LIBBUTANO is the main directory of butano library (https://github.com/GValiente/butano).
# PYTHON is the path to the python interpreter.
# SOURCES is a list of directories containing source code.
# INCLUDES is a list of directories containing extra header files.
# DATA is a list of directories containing binary data files with *.bin extension.
# GRAPHICS is a list of files and directories containing files to be processed by grit.
# AUDIO is a list of files and directories containing files to be processed by the audio backend.
# AUDIOBACKEND specifies the backend used for audio playback. Supported backends: maxmod, aas, null.
# AUDIOTOOL is the path to the tool used process the audio files.
# DMGAUDIO is a list of files and directories containing files to be processed by the DMG audio backend.
# DMGAUDIOBACKEND specifies the backend used for DMG audio playback. Supported backends: default, null.
# ROMTITLE is a uppercase ASCII, max 12 characters text string containing the output ROM title.
# ROMCODE is a uppercase ASCII, max 4 characters text string containing the output ROM code.
# USERFLAGS is a list of additional compiler flags:
#     Pass -flto to enable link-time optimization.
#     Pass -O0 or -Og to try to make debugging work.
# USERCXXFLAGS is a list of additional compiler flags for C++ code only.
# USERASFLAGS is a list of additional assembler flags.
# USERLDFLAGS is a list of additional linker flags:
#     Pass -flto=<number_of_cpu_cores> to enable parallel link-time optimization.
# USERLIBDIRS is a list of additional directories containing libraries.
#     Each libraries directory must contains include and lib subdirectories.
# USERLIBS is a list of additional libraries to link with the project.
# DEFAULTLIBS links standard system libraries when it is not empty.
# STACKTRACE enables stack trace logging when it is not empty.
# USERBUILD is a list of additional directories to remove when cleaning the project.
# EXTTOOL is an optional command executed before processing audio, graphics and code files.
#
# All directories are specified relative to the project directory where the makefile is found.
#---------------------------------------------------------------------------------------------------------------------
TARGET      	:=  gba-sokoban-bn
BUILD       	:=  build
LIBBUTANO   	:=  ../butano
PYTHON      	:=  python
SOURCES     	:=  src/game/src src/game/src/audio src/game/src/gfx src/game/src/save src/game/src/state src/game/src/game
INCLUDES    	:=  src/game/include src/game/src
DATA        	:=
GRAPHICS    	:=  build/asset_tmp/sprites/chara/chara build/asset_tmp/sprites/mini build/asset_tmp/sprites/ui build/asset_tmp/sprites/ui/common build/asset_tmp/sprites/ui/gallery build/asset_tmp/sprites/ui/icons build/asset_tmp/sprites/ui/menu build/asset_tmp/sprites/ui/paper build/asset_tmp/sprites/ui/practice build/asset_tmp/sprites/ui/title build/asset_tmp/stills/event build/asset_tmp/stills/gallery build/asset_tmp/stills/gallery/bgm build/asset_tmp/stills/gallery/event_select build/asset_tmp/stills/gallery/view_bustup build/asset_tmp/stills/gallery/view_still build/asset_tmp/stills/mainmenu build/asset_tmp/stills/practice build/asset_tmp/stills/save_attention build/asset_tmp/stills/soukoban_gamentest build/asset_tmp/stills/title
AUDIO       	:=  Asset/audio
AUDIOBACKEND	:=  maxmod
AUDIOTOOL		:=  
DMGAUDIO    	:=  dmg_audio
DMGAUDIOBACKEND	:=  default
ROMTITLE    	:=  SOKOBAN
ROMCODE     	:=  SOKB
USERFLAGS   	:=  
USERCXXFLAGS	:=  
USERASFLAGS 	:=  
USERLDFLAGS 	:=  
USERLIBDIRS 	:=  
USERLIBS    	:=  
DEFAULTLIBS 	:=  true
STACKTRACE		:=	
USERBUILD   	:=  
EXTTOOL     	:=  python src/tools/prebuild.py

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak
