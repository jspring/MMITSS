# Makefile
# Took out build for second executable with -DCOMPARE
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

SERVER_LIB = $(OBJDIR)/libserver_lib.a
SERVER_LIB_OBJS = $(OBJDIR)/server_lib.o

PATH_LIBS = -lserver_lib -ldl $(PATH_CORE_LIBS)
PATH_LDFLAGS += -L./lnx
PATH_CFLAGS += -I$(ATSC_INC_DIR) -pg

HDRS = fcs.h msgs.h ab3418comm.h ab3418_lib.h server_lib.h set_min_max_green.h

EXEC = $(OBJDIR)/data_manager $(OBJDIR)/server $(OBJDIR)/client

all: $(OBJDIR) $(SERVER_LIB) $(EXEC) 

$(EXEC): $(SERVER_LIB_OBJS)

$(SERVER_LIB): $(SERVER_LIB_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(SERVER_LIB_OBJS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	\rm -f $(OBJDIR)/*.[oa] *.*~ $(EXEC) $(ALGORITHM_DIR)/$(OBJDIR)/*.[oa]
