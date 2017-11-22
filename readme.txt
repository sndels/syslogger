Syslogger

This is a simple logging program with corresponding static library for the
interface. The client API is simply register, unregister and log calls with
a client id (really just the logging pipe) passed around. The daemon doesn't
notify it's clients expilicitly in case it closes before them as that would
add more pipes and somewhat redundant logic to the code. Thus, SIGPIPE is to
be ignored by the client as the library will detect a closed pipe by EPIPE
returned from a write and return an error from the log call to indicate the
daemon gone missing.

The daemon is implemented as a multi threaded application with three types
of threads: the main thread listens to a sysv message queue for client's
registering messages, each client's pipe is assigned to a single thread for
listening and finally a single writer logs the received messages to the main
log file. A thread safe signal handler takes care of detecting SIG_INT to
end logging and a thread safe queue implemented as a simple linked list keeps
the logging messages in order.

On a new read, the client's logging thread pushes a new empty message to the
queue with it's contents locked so that the length of the lock needed for the
queue is minimized. The message lock is released when the whole message has
been copied and at that point the writer thread can begin writing if it has
already popped the message. The queued message type has client info and the
message's timestamp in addition to the message text to help in the print out.

Current issues:
-Well, SIPIPE needs to be ignored but that seems like something that would
 be done anyway if the client uses pipes in it's own logic
-Subsequent messages of a single client can be concatenated if the client watcher
 thread has to wait for a queue lock for too long. However, logging is pretty
 resillient to stress testing, handling upwards of Y clients with X logging
 threads outputting once per millisecond with relatively few message pairs being
 combined (i7 6790k).
