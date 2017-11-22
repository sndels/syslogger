Syslogger

This is a simple logging program with corresponding static library for the
interface.

Usage
Just have syslogger running in one terminal [TODO: actual daemon?] and the client
should register itself before logging and unregister after. SIGPIPE needs to be
ignored in the client for the library error handling to work. Logging messages
are printed using the suggested format of for example

Nov 22 00:04:54.212 10318 thread0 this is a message
[date] [   time   ] [pid] [name]  [    message    ]

In testing (i7 4790k) syslogger could mostly handle as much traffic as I could
throw at it. The only failure case I observed was the concatenation of a client's
messages if multiple are written to the pipe while the listener is waiting for a
lock to push the last message to the output queue. This was expected but reproducing
it requires tens of messages per millisecond and even then only a small number of
pairs/triplets are combined. Another concern I had was the size of the message
queue building up as only a single writer is handling the messages pushed by all
of the listeners. This was indeed the case [VERIFY!!] but solving that is non-
trivial. The log will most likely not fit into memory, requiring constant remapping
to the active part if the memory mapping is used to do concurrent writes from multiple
threads. Aio_write would be another obvious choice but it seems like that will also
just queue up the writes in its own implementation so it would only add complexity.

Implementation

The client API is simply register, unregister and log calls with client info
(really just the logging pipe) passed around. The daemon doesn't notify it's clients
expilicitly in case it closes before them as that would add more pipes and somewhat
redundant logic to the code. The library will instead detect a closed pipe by EPIPE
returned from a write and return an error from the log call to indicate the daemon
gone missing. This is why SIGPIPE needs to be ignored as that would halt the client
with the default action.

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
-SIPIPE needs to be ignored but that seems like something that would be done anyway
 if the client uses pipes in it's own logic
-Concatenation of some messages if tens of them are received inside a ms
