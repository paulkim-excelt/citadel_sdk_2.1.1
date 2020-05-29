ifndef ROOT
ROOT=../..
endif

ifndef BUILD_FOR_I386
DEST_DIR=../../../../../../release/USHCD/linux/amd64
else
DEST_DIR=../../../../../../release/USHCD/linux/i386
endif

CC=gcc
OBJS=$(ROOT)/src/CVFWUpgrade/cvmain.o \
     $(ROOT)/src/CVFWUpgrade/cv_if.o \
     $(ROOT)/src/CVFWUpgrade/cnsl_utils.o

# The -I$(ROOT)/src/CVFWUpgrade needs to be removed when ready to ship
INCLUDES=  -I$(ROOT)/inc/common -I$(ROOT)/inc/CVFWUpgrade
#  -Wno-write-strings gets rid of the warnings triggered by assigning strings to str ptr arrays.
# Add -g in CFLAGS for debugging
CFLAGS= -Wall -Wno-write-strings -fPIC
LIBS=${ROOT}/lib/libCVUsrLib.a
SOLIBS=CVUsrIfc

all : $(OBJS)

%.o:%.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

CVFWUpgrade : $(OBJS)
	$(CC) -rdynamic -g -o $@ $(OBJS) $(LIBS) -L ${ROOT}/lib -ldl -l$(SOLIBS) -pthread
	if [ ! -d ${ROOT}/bin ]; then mkdir ${ROOT}/bin; fi
	mv $@ $(ROOT)/bin/.

install:
	cp $(ROOT)/bin/CVFWUpgrade $(DEST_DIR)/bin/CVFWUpgrade
 
clean:
	rm -f $(OBJS) $(ROOT)/src/CVFWUpgrade/*.*swp $(ROOT)/bin/CVFWUpgrade
