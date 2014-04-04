# Makefile_Linux	|	Sep 08 2011
#
#	This is the Linux makefile for the StolTest plugin.
#
# Document Author: William Yu
# Document Contributor(s):
#
# Revision History

include $(CAPATH_MK_DEFS)

CC = g++
BASEDIR = /opt/xojo/xojo2014r1
PLUGINSDKDIR = $(BASEDIR)/Extras/Plugins\ SDK
SDKGLUECODEDIR = $(PLUGINSDKDIR)/Glue\ Code
SDKINCLUDEDIR = $(PLUGINSDKDIR)/Includes
INCLUDE =	-I$(SDKINCLUDEDIR)

PREFIXHEADER = $(SDKINCLUDEDIR)/LinuxHeader.h
CFLAGS = -s -O2 -DIGNOREQT -D__INTEL__ -DLINUX=1 -D__GCC__ $(INCLUDE) -include $(PREFIXHEADER)
CFLAGS += -m32 -march=i586 -g -std=c++0x -Wl,--export-dynamic -fPIC
CFLAGS += $(PATH_CFLAGS) -I$(ATSC_INC_DIR)
BUILDOBJDIR = BuildLinux
SONAME = mmitss_ui

OBJS =	$(BUILDOBJDIR)/PluginMain.o \
	$(BUILDOBJDIR)/$(SONAME).o 

all: CreateBuildObjDir $(OBJS)
	g++ -shared -fPIC -Wl,--no-undefined BuildLinux/mmitss_ui.o -o mmitss_ui.so BuildLinux/PluginMain.o $(PATH_LDFLAGS) $(PATH_LIBS)

CreateBuildObjDir:
	mkdir -p $(BUILDOBJDIR)

$(BUILDOBJDIR)/$(SONAME).o: $(SONAME).cpp
	$(CC) $(CFLAGS) $(SONAME).cpp -c -o $@

$(BUILDOBJDIR)/PluginMain.o: $(SDKGLUECODEDIR)/PluginMain.cpp
	$(CC) $(CFLAGS) $(SDKGLUECODEDIR)/PluginMain.cpp -c -o $@

clean:
	rm $(BUILDOBJDIR)/*.o -f
	rm $(SONAME).so $(BASEDIR)/Plugins/$(SONAME).so

install:
	cp $(SONAME).so $(BASEDIR)/Plugins
