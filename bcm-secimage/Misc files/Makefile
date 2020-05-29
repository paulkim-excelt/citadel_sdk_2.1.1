##################################################
# Makefile for Creation of Pheonix Boot Image  
# NOTE: you should login to xrhel in order to build it
##################################################

TARGET		= secimage

OBJS		= secimage.o crypto.o io.o misc.o sbi.o chain.o hmac.o dauth.o
OBJS += ../../qlip/q_addsub.o\
        ../../qlip/q_dsa.o    \
        ../../qlip/q_fdivrem.o \
        ../../qlip/q_lip_utils.o  \
        ../../qlip/q_mulsqr.o  \
        ../../qlip/q_rsa.o \
        ../../qlip/q_dh.o  \
        ../../qlip/q_ecdsa.o  \
        ../../qlip/q_inv.o    \
        ../../qlip/q_mod.o    \
        ../../qlip/q_pka_hw.o  \
        ../../qlip/q_rsa_4k.o  \
        ../../qlip/q_div.o     \
        ../../qlip/q_ecp.o    \
        ../../qlip/q_lip_aux.o  \
        ../../qlip/q_modexp.o    \
        ../../qlip/q_ppsel.o

INCLUDES	= secimage.h 
CC			= gcc
LD			= ld
MV			= mv
CP			= cp
RM			= rm -f

#######################################
# Need to specify the location of the OpenSSL directory
#######################################
#this is the default openssl version 0.98d installation location in simba
#SSLDIR		= /projects/sentry5/openssl098d
#SSLDIR		= /usr/local/openssl098d

# this is the directory for xrhel
SSLDIR		= /tools/oss/packages/x86_64-rhel3/openssl/0.9.8d
CFLAGS		= -Wall -g -DLINUX_BUILD
#-O2 -g 

# We now use static linked library (.a) instead of dynamic, otherwise it will not find library
#LIBS		= -lssl -ldl -lcrypto -L$(SSLDIR)/lib
LIBS		= $(SSLDIR)/lib/libssl.a -ldl $(SSLDIR)/lib/libcrypto.a -L$(SSLDIR)/lib

IFLAGS		= -I$(SSLDIR)/include -I../../include

#######################################
# Uncomment the following lines for building on SunOS
# Also change SSLDIR above
#######################################
#CFLAGS		+= -DSUN_OS
#LDFLAGS	+=  -R$(SSLDIR)/lib 
#LIBS		+= -lc -lm -lsocket

#######################################
# Implicit Rules
#######################################
%.o : %.c
	$(CC) -c $(CFLAGS) $(IFLAGS) -o $@ $< 

all: $(TARGET)

$(TARGET): $(OBJS) $(INCLUDES)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

clean: 
	$(RM) $(TARGET) $(OBJS) 
