# Makefile
# Took out build for second executable with -DCOMPARE
#
# Copyright (c) 2008   Regents of the University of California
#

SERVER_LIB = $(OBJDIR)/libserver_lib.a
SERVER_LIB_OBJS = $(OBJDIR)/server_lib.o

PATH_LIBS = -lserver_lib -ldl -lrt
PATH_LDFLAGS += -L./lnx
PATH_CFLAGS += -I$(ATSC_INC_DIR) -pg
PATH_OBJS = $(OBJDIR)/timestamp.o \
	$(OBJDIR)/sys_rt_linux.o \
	$(OBJDIR)/ab3418_libudp.o \
	$(OBJDIR)/udp_utils.o

OBJDIR = lnx

EXEC = $(OBJDIR)/data_manager $(OBJDIR)/server $(OBJDIR)/client $(OBJDIR)/ab3418commudp

all: $(OBJDIR) $(SERVER_LIB) $(PATH_OBJS) $(EXEC) 

$(OBJDIR)/%.o: %.c
	gcc -c -DGNU_SOURCE -Wall -pg -I. $*.c -o $@

.o: $(SERVER_LIB_OBJS)
	gcc -L./lnx -o $@ $@.o $(PATH_OBJS) $(PATH_LIBS)

$(SERVER_LIB): $(SERVER_LIB_OBJS)
	ar -rv $@ $(SERVER_LIB_OBJS) 

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	\rm -f $(OBJDIR)/*.[oa] *.*~ $(EXEC) $(ALGORITHM_DIR)/$(OBJDIR)/*.[oa]
