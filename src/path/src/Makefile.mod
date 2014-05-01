# Makefile
#
# Copyright (c) 2013   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

METER_LIB = $(OBJDIR)/libmeter_lib.a
METER_LIB_OBJS = $(OBJDIR)/meter_lib.o

PATH_LDFLAGS += -L./lnx

PATH_CFLAGS += -I$(ATSC_INC_DIR) -pg -Wall
PATH_OBJS += $(OBJDIR)/server_lib.o

PATH_LDFLAGS += -L$(ATSC_LIB_DIR)
PATH_LIBS += -lset_min_max_green_lib

EXEC = $(OBJDIR)/data_manager

all: 	$(OBJDIR) $(PATH_OBJS) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	\rm -f $(OBJDIR)/*.[oa] $(EXEC)
