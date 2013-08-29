#
# Copyright (C) 2013 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
ROOT    := $(realpath ../..)
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
