TARGET = homebrewSorter
OBJS = src/fileOperation.o src/main.o src/iso_common.o src/isoreader.o

#To build for custom firmware:
BUILD_PRX = 1
PSP_FW_VERSION=661

INCDIR = ./include
CFLAGS = -Os -frename-registers -G0 -Wall -D__USER__
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR = ./libs
STDLIBS= -losl -lpng -lz -lm -ljpeg \
         -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu \
		 -lpspgum  -lpspaudiolib -lpspaudio -lpsphttp -lpspssl -lpspwlan \
         -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl
CFWLIBS= -lpspsystemctrl_user -lpspsysc_user
LIBS = $(STDLIBS) $(CFWLIBS)
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Homebrew Sorter
PSP_EBOOT_ICON = ICON0.PNG

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
