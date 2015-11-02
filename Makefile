ifeq (${DEBUG},1)
CFLAGS = -O0 -ggdb
else
CFLAGS = -O2
endif

INCL = ./include
LDFLAGS = -lpthread -L. -lsstm
SRCPATH = ./src

default: libsstm.a
	cc ${CFLAGS} -I${INCL} src/bank.c -o bank ${LDFLAGS}
	cc ${CFLAGS} -I${INCL} src/ll.c -o ll ${LDFLAGS}

clean:
	rm bank *.o src/*.o


$(SRCPATH)/%.o:: $(SRCPATH)/%.c include/sstm.h include/sstm_alloc.h
	cc $(CFLAGS) -I${INCL} -o $@ -c $<

.PHONY: libsstm.a

libsstm.a:	src/sstm.o src/sstm_alloc.o
	ar cr libsstm.a src/sstm.o src/sstm_alloc.o

