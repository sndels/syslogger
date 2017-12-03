Syslogger

This is a simple logging program with corresponding static library for clients to
interface with.

Usage
Just have syslogger running in one terminal and clients should register to start
logging and unregister after. SIGPIPE needs to be ignored in the client for the
library to detect if the daemon has exited. Logging messages are printed using
the suggested format of

Nov 22 00:04:54.212 10318 thread0 this is a message
[date] [   time   ] [pid] [name ] [    message    ]

In testing (an i7 system) syslogger could handle ten clients each pushing messages
with 5ms intervals. The failure case I observed was concatenation of a client's
messages if multiple are written to the pipe while the listener is waiting for a
lock to push a message to the output queue. This behaviour is not deterministic
as it results from the undefined order of ptrhead mutex locks for multiple waiting
threads. A sample of a million messages from 20 clients with 1ms waits resulted in
a few hundred concatenated messages and the amount of messages concatenated into
one varied from 2 to 10.

Implementation

The client API is simply register, unregister and log calls with client info
(really just the logging pipe) passed around. The daemon doesn't notify it's clients
expilicitly in case it closes before them as that would add more pipes and somewhat
redundant logic to the code. The library will instead detect a closed pipe by EPIPE
returned from a write and return an error from the log call to indicate the daemon
has gone missing. This is why SIGPIPE needs to be ignored as that would halt the
client with the default action.

The daemon is implemented as a multi-threaded application with three types of threads:
the main thread listens to a sysv message queue for client's registering messages,
each client's pipe is assigned to a new thread for listening and finally a single
writer logs the received messages to the main log file. A thread safe signal handler
takes care of detecting SIG_INT to end logging and a thread safe queue implemented
as a simple linked list keeps the logging messages in order.

On a new read, the pipe thread thread pushes a new empty message to the
queue with it's contents locked so that the length of the lock needed for the
queue is minimized. The message lock is released when the whole message has
been copied and at that point the writer thread can begin writing if it has
already popped the message. The queued message type has client info and the
message's timestamp in addition to the message text to help in the print out.

Current issues:
-SIPIPE needs to be ignored but that seems like something I'd do anyway if the
 client uses pipes in it's own logic
-Concatenation of messages with heavy loads
-test hangs waiting for stdin-input even if daemon has exited, seems small enough
 to not implement more complex exit logic
