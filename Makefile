# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -Wall -pedantic

DAEMON_SRC=daemon/%.c
DAEMON_OBJ=main.o

DAEMONLIB_SRC=daemon_lib/%.c
DAEMONLIB_OBJ=log_interface.o

all: syslogger syslogger_lib_test

obj/daemon/%.o: $(DAEMON_SRC)
		@mkdir -p $(@D)
		$(CC) -c -o $@ $< $(CFLAGS)

obj/daemon_lib/%.o: $(DAEMONLIB_SRC)
		@mkdir -p $(@D)
		$(CC) -c -o $@ $< $(CFLAGS)

syslogger: obj/daemon/$(DAEMON_OBJ)
		gcc -o $@ $^

syslogger_lib_test: obj/daemon_lib/$(DAEMONLIB_OBJ) obj/daemon_lib/main.o
		gcc -o $@ $^

.PHONY: clean

clean:
		rm -rf obj
