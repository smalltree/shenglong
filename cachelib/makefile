all: cacheLsn2eALib.o  cacheLsn2eLib.o
		armips -ruv C:\Tornado2.2\target\lib\mips\MIPS64\commonle_lsn2e\libarch.a cacheLsn2eALib.o  cacheLsn2eLib.o
clean:
		rm -f *.o
cacheLsn2eALib.o:cacheLsn2eALib.s
		ccmips -G 0 -mno-branch-likely -mips3 -EL -ansi -fno-builtin -O2 -I/h   -I. -IC:\Tornado2.2\target\config\all -IC:\Tornado2.2\target/h -IC:\Tornado2.2\target/src/config -IC:\Tornado2.2\target/src/drv -DCPU=MIPS64 -DTOOL_FAMILY=gnu -DTOOL=gnule -DMIPSEL   -P -xassembler-with-cpp -DMIPSEL -c -o cacheLsn2eALib.o cacheLsn2eALib.s

cacheLsn2eLib.o:cacheLsn2eLib.c
		ccmips -c -G 0 -mno-branch-likely -mips3 -EL -ansi -fno-builtin -O2 -Wall -I/h -I. -IC:\Tornado2.2\target\config\all -IC:\Tornado2.2\target/h -IC:\Tornado2.2\target/src/config -IC:\Tornado2.2\target/src/drv -DCPU=MIPS64 -DTOOL_FAMILY=gnu -DTOOL=gnule -DMIPSEL   -c -o cacheLsn2eLib.o cacheLsn2eLib.c