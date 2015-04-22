# Makefile
# Took out build for second executable with -DCOMPARE
#
# Copyright (c) 2008   Regents of the University of California
#

SERVER_LIB = $(OBJDIR)/libserver_lib.a
SERVER_LIB_OBJS = $(OBJDIR)/server_lib.o
AZ_INC_DIR = ./module.asn1-2
AZ_LD_DIR = ./module.asn1-2
#MMITSS_DIR = /home/mmitss
#SAVARI_INC = /home/jspring/savari/SDK_toolchain_and_Images/RSE/savari_sdk/toolchain/include/c++/4.1.2
#SAVARI_OTHER = $(SAVARI_INC)/i386-uclibc/
#PERF_OBS_DIR = $(MMITSS_DIR)/source/MRP/MRP_PerformanceObserver/Linux/MMITSS_rsu_PerformanceObserver_TT

PATH_LIBS = -lserver_lib -ldl -lrt
PATH_LDFLAGS += -L./lnx -L$(AZ_LD_DIR)
#PATH_CFLAGS += -Wall -pg -I. -I$(AZ_INC_DIR) -I$(PERF_OBS_DIR) -I$(SAVARI_INC) -I$(SAVARI_OTHER) -Iinclude -fpermissive
PATH_CFLAGS += -Wall -pg -I. -I$(AZ_INC_DIR)
PATH_OBJS = $(OBJDIR)/timestamp.o \
	$(OBJDIR)/sys_rt_linux.o \
	$(OBJDIR)/ab3418_libudp.o \
	$(OBJDIR)/tcp_utils.o \
	$(OBJDIR)/udp_utils.o 

OBJDIR = lnx

EXEC =$(OBJDIR)/server $(OBJDIR)/client $(OBJDIR)/ab3418commudp $(OBJDIR)/sim_spat $(OBJDIR)/data_manager  $(OBJDIR)/sendSoftCall

all: $(OBJDIR) $(SERVER_LIB) $(PATH_OBJS) $(EXEC) 

$(OBJDIR)/%.o: %.c
	gcc -c -DGNU_SOURCE $(PATH_CFLAGS) $*.c -o $@ -I$(AZ_INC_DIR)
#	gcc -c -DGNU_SOURCE $(PATH_CFLAGS) $*.c -o $@ $(AZ_LD_DIR)/der_encoder.o

#$(OBJDIR)/%.o: %.c
#	g++ -c -DGNU_SOURCE $(PATH_CFLAGS) $*.c -o $@ -I$(AZ_INC_DIR)
#	gcc -c -DGNU_SOURCE $(PATH_CFLAGS) $*.c -o $@ $(AZ_LD_DIR)/der_encoder.o

.o: $(SERVER_LIB_OBJS)
	gcc -L./lnx -o $@ $@.o $(PATH_OBJS) $(PATH_LIBS) -L$(AZ_LD_DIR) $(AZ_LD_DIR)/*.o
#	gcc -L./lnx -o $@ $@.o $(PATH_OBJS) $(PATH_LIBS) $(AZ_LD_DIR)/der_encoder.o

$(SERVER_LIB): $(SERVER_LIB_OBJS)
	ar -rv $@ $(SERVER_LIB_OBJS) 

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJDIR)/*.[oa] *.*~ $(EXEC) $(ALGORITHM_DIR)/$(OBJDIR)/*.[oa]
#	\rm -f $(OBJDIR)/*.[oa] *.*~ $(EXEC) $(ALGORITHM_DIR)/$(OBJDIR)/*.[oa]
