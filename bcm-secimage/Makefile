TARGET		= secimage
OBJS		= chain.o crypto.o dauth.o getopt.o hmac.o io.o \
              misc.o q_addsub.o q_div.o q_fdivrem.o q_inv.o q_lip_aux.o  \
              q_lip_utils.o q_mod.o q_modexp.o q_mulsqr.o q_rsa_4k.o q_rsa.o sbi.o secimage.o

INCLUDES	= secimage.h
CC			= gcc
LD			= ld
MV			= mv
CP			= cp
RM			= rm -f

SSLDIR		= ./openssl-1.0.1g
GMPDIR		= ./gmp-6.0.0/.libs

# NB: -D's should match SecimageWin.vcxproj <PreProcessorDefinitions>
CFLAGS		= -Wall -g -DLINUX_BUILD -DQLIP_USE_GMP -m32

LIBS		= $(GMPDIR)/libgmp.a $(SSLDIR)/libssl.a -ldl $(SSLDIR)/libcrypto.a -L$(SSLDIR)/lib

IFLAGS		= -I$(SSLDIR)/include -I../../include
LDFLAGS     = -m32

%.o : %.c
	$(CC) -c $(CFLAGS) $(IFLAGS) -o $@ $<

all: openssl gmp $(TARGET) big2little

gmp: gmp-6.0.0a.tar
	test -d gmp-6.0.0 || tar xfv gmp-6.0.0a.tar ; cd gmp-6.0.0 ; ABI=32 ./configure; make

openssl: openssl-1.0.1g.tar.gz
	test -d openssl-1.0.1g || tar xfv openssl-1.0.1g.tar.gz; cd openssl-1.0.1g ; setarch i386 ./config -m32 --prefix=$(CURDIR)/openssl --openssldir=$(CURDIR)/openssl ; make ; make install_sw

big2little: big2little.c
	$(CC) -o $@ big2little.c

$(TARGET): $(OBJS) $(INCLUDES)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS) -ldl

clean:
	$(RM) $(TARGET) $(OBJS) big2little.o
	rm -rf openssl openssl-1.0.1g gmp-6.0.0
