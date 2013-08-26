# makestuff/apps/mul/Makefile
ROOT    := /c/makestuff
DEPS    := 
TYPE    := exe
SUBDIRS := 

#cl -D UNICODE -D _UNICODE -D _WIN32_IE=0x0500 -D WINVER=0x600 -Os -I../../common -EHsc main.cpp 
#kernel32.lib user32.lib ole32.lib shell32.lib uuid.lib advapi32.lib; cp main.exe /w

EXTRA_CFLAGS := -D UNICODE -D _UNICODE -D _WIN32_IE=0x0500 -D WINVER=0x600
LINK_EXTRALIBS_REL := ole32.lib shell32.lib uuid.lib advapi32.lib
LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)

-include $(ROOT)/common/top.mk
