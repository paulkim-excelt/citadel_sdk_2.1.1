#Paths
ifndef ROOT
ROOT=../..
endif

DEST_DIR=../../../../../../release/USHCD/linux/amd64

CC=gcc
OBJS=$(ROOT)/src/CVUserInterface/CVUserInterface.o \
     $(ROOT)/src/CVUserInterface/CVuserInterfaceUtils.o \
     $(ROOT)/src/CVUserInterface/CVLogger.o \
     $(ROOT)/src/CVUserInterface/CVCmdStateList.o \
     $(ROOT)/src/CVUserInterface/CVUIThreadHandlers.o
INCLUDES=  -I$(ROOT)/inc/common -I$(ROOT)/inc/CVUserInterface 
#Add -g for debugging
CFLAGS= $(INCLUDES) -Wall -fPIC
LIBS=${ROOT}/lib/libCVUsrIfc.so
all:$(LIBS)

$(LIBS):$(OBJS)
	if [ ! -d ${ROOT}/lib ]; then mkdir ${ROOT}/lib; fi
	$(CC) -shared $(OBJS) -lc -lpthread -lusb-1.0 -o $@
#	ld -shared -o $@ -lc -lpthread $(OBJS) 
#	ld -shared -soname libCVUsrIfc.so.1 -o libCVUsrIfc.so.1.0 -lc -lpthread $(OBJS)
 
install:
	cp $(LIBS) $(DEST_DIR)/lib/libCVUsrIfc.so
 
clean:
	rm -fr $(OBJS) $(LIBS)
#	rm -fr ./*.o ./.*~ ./.*swp ./lib*.so.*

