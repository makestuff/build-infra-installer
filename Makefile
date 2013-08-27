ROOT    := /c/makestuff
DEPS    := 
TYPE    := exe
SUBDIRS := 
PRE_BUILD := resource
POST_BUILD := manifest

EXTRA_CFLAGS := -D UNICODE -D _UNICODE -D _WIN32_IE=0x0500 -D WINVER=0x600
LINK_EXTRALIBS_REL := ole32.lib shell32.lib advapi32.lib comctl32.lib win.$(MACHINE)/resource.res -subsystem:windows
LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)

-include $(ROOT)/common/top.mk

resource: resource.rc
	mkdir -p $(PM)
	rc -fo$(PM)/resource.res $<

manifest: $(OUTDIR_REL)/$(TARGET) $(OUTDIR_DBG)/$(TARGET)
	cp application.manifest $(OUTDIR_REL)/$(TARGET).intermediate.manifest
	mt.exe -nologo -manifest $(OUTDIR_REL)/$(TARGET).intermediate.manifest "-outputresource:$(OUTDIR_REL)/$(TARGET);#1"
	cp application.manifest $(OUTDIR_DBG)/$(TARGET).intermediate.manifest
	mt.exe -nologo -manifest $(OUTDIR_DBG)/$(TARGET).intermediate.manifest "-outputresource:$(OUTDIR_DBG)/$(TARGET);#1"
