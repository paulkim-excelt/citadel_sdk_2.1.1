#Paths
ifndef ROOT
ROOT=../..
endif

DEST_DIR=../../../../../../release/USHCD/linux/amd64

CC=gcc
OBJS:=$(ROOT)/src/CVUsrLib/HelperFunctions.o \
      $(ROOT)/src/CVUsrLib/CVList.o \
      $(ROOT)/src/CVUsrLib/CVUtil.o \
      $(ROOT)/src/CVUsrLib/CVUsrLib.o \
      $(ROOT)/src/CVUsrLib/CVLogger.o
TST_OBJS:=test_obj.o
INCLUDES+= -I$(ROOT)/inc/common -I$(ROOT)/inc/CVUsrLib
#Add -g for debugging
CFLAGS+= $(INCLUDES) -Wall -fPIC
LIBS=${ROOT}/lib/libCVUsrLib.a

all:$(LIBS)

$(LIBS):$(OBJS)
	if [ ! -d ${ROOT}/lib ]; then mkdir ${ROOT}/lib; fi
	$(AR) -rc $(LIBS) $(OBJS)

tst_app:$(TST_OBJS)
	$(CC) -o $@ $(TST_OBJS) $(LIBS) -ldl

install:
	cp $(LIBS) $(DEST_DIR)/lib/libCVUsrLib.a

clean:
	rm -fr $(OBJS) $(LIBS)
#	rm -fr ./*.o ./*.a ./.*~ ./.*swp

