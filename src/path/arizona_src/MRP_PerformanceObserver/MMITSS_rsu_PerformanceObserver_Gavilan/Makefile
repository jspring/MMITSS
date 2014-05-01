SRC =   
SRC += MMITSS_rsu_PerformanceObserver_Gavilan.cpp AccelerationSet4Way.cpp BasicVehicle.cpp Brakes.cpp GeoCoord.cpp NMAP.cpp PositionLocal3D.cpp

TGT:=MMITSS_rsu_PerformanceObserver_Gavilan
TOOLCHAIN=../toolchain
TOOLCHAIN=/usr

SAVARI_CC=$(TOOLCHAIN)/bin/g++
SAVARI_LD=$(TOOLCHAIN)/bin/g++
CFLAGS:= -Wall -DPRINT_LOG -std=c++0x -fpermissive
LDFLAGS:=
INCLUDES:=-I. -I/include -I/usr/include
#LIBS =-L/lib -L/usr/lib -lstdc++ -luClibc++ -leloop -lnetsnmp
LIBS =-L/lib -L/usr/lib -lstdc++ 

OBJ = $(patsubst %.cpp, %.o, ${SRC})

LIBS +=
all:	$(TGT)
$(TGT): $(OBJ)
	${SAVARI_CC}  $(LDFLAGS) $(OBJ) -o $(TGT) $(LIBS)

%.o: %.cpp
	${SAVARI_CC} $(INCLUDES) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(TGT) $(OBJ) *~ *.o
