# Name of the binary
PROG = twitterbot

# All of the C files we'll need go here, but with a .o extension
OBJS = conf.o callbacks.o main.o

LIBS = glib-2.0 gthread-2.0 libconfig oauth libcurl json-glib-1.0

CC = gcc
CFLAGS = -ggdb2 -Wall -Werror -Wextra -I${CURDIR}/libircclient -DTWITTERBOT_VERSION='"$(shell hg id -n)"' $(shell pkg-config --cflags ${LIBS})
LDFLAGS = -Wl,-rpath,${CURDIR}/libircclient -L${CURDIR}/libircclient $(shell pkg-config --libs ${LIBS}) -lircclient

all: ${PROG}

${PROG}: ${OBJS}
	@echo "Building $@"
	@${CC} -o $@ ${OBJS} ${LDFLAGS}

global.h: libircclient conf.h

libircclient:
	@${MAKE} -C libircclient DEBUG=1

%.o: %.c Makefile global.h
	@echo "Compiling $<"
	@${CC} ${CFLAGS} -c $< -o $@

%.i: %.c Makefile global.h
	@echo "Prepocessing $<"
	@${CC} ${CFLAGS} -E $< -o $@

run: ${PROG}
	@echo "Running ${PROG}"
	@./${PROG}

clean:
	@echo "Cleaning ${PROG}"
	@rm -f ${PROG} ${OBJS} $(wildcard auth/*/auth.so auth/*/*/auth.so)
	@${MAKE} -C libircclient clean

leakcheck: ${PROG}
	@echo "Leak-checking ${PROG}"
	@MALLOC_CHECK_=1 G_SLICE="all" G_DEBUG="all" valgrind --leak-check=full --track-fds=yes --show-reachable=yes --track-origins=yes ./${PROG}

.PHONY: all libircclient run clean leakcheck
