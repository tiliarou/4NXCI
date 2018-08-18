include config.mk

.PHONY: clean

INCLUDE = -I ./mbedtls/include
LIBDIR = ./mbedtls/library
CFLAGS += -D_BSD_SOURCE -D_POSIX_SOURCE -D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE -D__USE_MINGW_ANSI_STDIO=1 -D_FILE_OFFSET_BITS=64

all:
	cd mbedtls && $(MAKE) lib
	$(MAKE) 4nxci

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) -o $@ $<

4nxci: sha.o aes.o extkeys.o pki.o hfs0.o utils.o nsp.o nca.o dummy.o cnmt.o xci.o main.o filepath.o ConvertUTF.o
	$(CC) -o $@ $^ $(LDFLAGS) -L $(LIBDIR)

aes.o: aes.h types.h

extkeys.o: extkeys.h types.h settings.h

filepath.o: filepath.c types.h

hfs0.o: hfs0.h types.h

main.o: main.c pki.h types.h version.h

pki.o: pki.h aes.h types.h

nsp.o: nsp.h

dummy.o: dummy.h dummy_files.h

cnmt.o: cnmt.h

nca.o: nca.h aes.h sha.h bktr.h filepath.h types.h pfs0.h npdm.h nca0_romfs.h

sha.o: sha.h types.h

utils.o: utils.h types.h

xci.o: xci.h types.h hfs0.h

ConvertUTF.o: ConvertUTF.h

clean:
	rm -f *.o 4nxci 4nxci.exe

clean_full:
	rm -f *.o 4nxci 4nxci.exe
	cd mbedtls && $(MAKE) clean

dist: clean_full
	$(eval NXCIVER = $(shell grep '\bNXCI_VERSION\b' version.h \
		| cut -d' ' -f3 \
		| sed -e 's/"//g'))
	mkdir 4nxci-$(NXCIVER)
	cp -R *.c *.h config.mk.template Makefile README.md LICENSE mbedtls 4nxci-$(NXCIVER)
	tar czf 4nxci-$(NXCIVER).tar.gz 4nxci-$(NXCIVER)
	rm -r 4nxci-$(NXCIVER)

