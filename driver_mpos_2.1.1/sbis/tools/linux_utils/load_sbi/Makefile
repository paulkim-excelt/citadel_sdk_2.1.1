ROOT=.
all:
	make all -C $(ROOT)/src/CVUserInterface/
	make all -C $(ROOT)/src/CVUsrLib/
	make CVFWUpgrade -C $(ROOT)/src/CVFWUpgrade/

install:
	make install -C $(ROOT)/src/CVUserInterface/
	make install -C $(ROOT)/src/CVUsrLib/
	make install -C $(ROOT)/src/CVFWUpgrade/

clean:
	make clean -C $(ROOT)/src/CVUserInterface/
	make clean -C $(ROOT)/src/CVUsrLib/
	make clean -C $(ROOT)/src/CVFWUpgrade/
