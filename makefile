TARGET = homebrewSorter
OBJS = src/fileOperation.o src/main.o

#To build for custom firmware:
BUILD_PRX = 1
PSP_FW_VERSION=661

CFLAGS = -Os -frename-registers -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR = ./oslib
STDLIBS= -losl -lpng -lz -lm -ljpeg \
         -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspgum  -lpspaudiolib -lpspaudio -lpsphttp -lpspssl -lpspwlan \
         -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl
LIBS = $(STDLIBS)
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Homebrew Sorter
PSP_EBOOT_ICON = ICON0.PNG
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
