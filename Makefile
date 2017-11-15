# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -Wall -pedantic

DAEMON_SRC_DIR=daemon
DAEMON_OBJ=log_routine.o main_daemon.o

DAEMONLIB_SRC_DIR=daemon_lib
DAEMONLIB_OBJ= log_interface.o main_lib_test.o

all: syslogger syslogger_lib_test

%.o: $(DAEMON_SRC_DIR)/%.c
		@mkdir -p $(@D)
		$(CC) -c -o $@ $< $(CFLAGS)

%.o: $(DAEMONLIB_SRC_DIR)/%.c
		@mkdir -p $(@D)
		$(CC) -c -o $@ $< $(CFLAGS)

syslogger: $(DAEMON_OBJ)
		gcc -o $@ $^

syslogger_lib_test: $(DAEMONLIB_OBJ)
		gcc -o $@ $^

.PHONY: clean

clean:
		rm -f *.o
