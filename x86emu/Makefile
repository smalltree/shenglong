all: x86emu.a

obj=x86emu
INC=-I$(obj)\include

OBJS = $(obj)\freebiosvga.o $(obj)\biosemu.o $(obj)\decode.o $(obj)\sys.o \
       $(obj)\besys.o $(obj)\prim_ops.o $(obj)\bios.o $(obj)\ops.o $(obj)\fpu.o \
       $(obj)\ops2.o $(obj)\prom_debug.o $(obj)\debug.o


x86emu.a: $(OBJS)
	armips -rcu $@ $^
	cp $@ $(obj)\$@

$(obj)\freebiosvga.o: $(obj)\freebiosvga.c
	$(CC) $(CFLAGS) -Os $(INC) -c -o $@ $(obj)\freebiosvga.c

$(obj)\biosemu.o: $(obj)\biosemu.c
	$(CC) $(CFLAGS) -c -o $@ $(obj)\biosemu.c

$(obj)\decode.o: $(obj)\decode.c
	$(CC) $(CFLAGS) -c -o $@ $(obj)\decode.c

$(obj)\sys.o: $(obj)\sys.c
	$(CC) $(CFLAGS) -c -o $@ $(obj)\sys.c

$(obj)\besys.o: $(obj)\besys.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $(obj)\besys.c

$(obj)\prim_ops.o: $(obj)\prim_ops.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $(obj)\prim_ops.c

$(obj)\bios.o: $(obj)\bios.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $(obj)\bios.c

$(obj)\ops.o: $(obj)\ops.c
	$(CC) $(CFLAGS) -Os $(INC) -c -o $@ $(obj)\ops.c

$(obj)\fpu.o: $(obj)\fpu.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $(obj)\fpu.c

$(obj)\ops2.o: $(obj)\ops2.c
	$(CC) $(CFLAGS) -Os $(INC) -c -o $@ $(obj)\ops2.c

$(obj)\debug.o: $(obj)\debug.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $(obj)\debug.c

$(obj)\prom_debug.o: $(obj)\prom_debug.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $(obj)\prom_debug.c

clean:XXXclean 
XXXclean:
	-$(RM) $(OBJS)
	-$(RM) $(obj)\x86emu.a
	-$(RM) x86emu.a
