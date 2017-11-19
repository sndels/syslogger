# Base from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS= -g -Wall -pedantic

DAEMON_SRC_DIR=daemon
DAEMON_OBJ=signal_handler.o logmsg_queue.o log_routine.o write_routine.o main_daemon.o

COMMON_SRC_DIR=common
COMMON_OBJ=sysv_messaging.o

DAEMONLIB_SRC_DIR=daemon_lib
DAEMONLIB_OBJ=log_interface.o

TESTAPP_SRC_DIR=test_app
TESTAPP_OBJ=main_test.o
DAEMON_DEPS=-pthread

all: syslogger libsyslogger.a syslogger_lib_test test

%.o: $(DAEMON_SRC_DIR)/%.c
		$(CC) -c -o $@ $< $(CFLAGS)

%.o: $(DAEMONLIB_SRC_DIR)/%.c
		$(CC) -c -o $@ $< $(CFLAGS)

%.o: $(COMMON_SRC_DIR)/%.c
		$(CC) -c -o $@ $< $(CFLAGS)

%.o: $(TESTAPP_SRC_DIR)/%.c
		$(CC) -c -o $@ $< $(CFLAGS)

syslogger: $(DAEMON_OBJ) $(COMMON_OBJ)
		gcc -pthread -o $@ $^

libsyslogger.a: $(COMMON_OBJ) $(DAEMONLIB_OBJ)
		ar rcs $@ $^

syslogger_lib_test: main_lib_test.o
		gcc -o $@ $^ -L. -lsyslogger

test: $(TESTAPP_OBJ)
		gcc -pthread -o $@ $^ -L. -lsyslogger

.PHONY: clean

clean:
		rm -f *.o *.a
