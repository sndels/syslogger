# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -Wall -pedantic

DAEMON_SRC_DIR=daemon
DAEMON_OBJ=signal_handler.o log_routine.o main_daemon.o
DAEMON_DEPS=-pthread

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
		gcc $(DAEMON_DEPS) -o $@ $^

syslogger_lib_test: $(DAEMONLIB_OBJ)
		gcc -o $@ $^

.PHONY: clean

clean:
		rm -f *.o
