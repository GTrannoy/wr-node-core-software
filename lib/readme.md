Library Overview {#mainpage}
================
This is the **Mock Turtle** (*TRTL*) library documentation. Here you can find all
the information about the *Mock Turtle* API and the main
library behavior that you need to be aware of.

If you are reading this from the doxygen documentation, then you can find
the API documentation in the usual Doxygen places. Otherwise, you can get
the API documentation directly from the source code that you can find in
the *lib* directory.

In this document we are going to provides you some clues to understand how
to use the library API.

Initialization
==============
To be able to use this library the first thing to do is to initialize a library
instance using trtl_init(); form this point on you are able to use the
library API. Of course, when you finished to use the library you have to
remove this instance using trtl_exit().

At the beginning, all communication channels are close, so before start to
communicate with the Real Time application you have to open your communication
channel. Then, close it when you have done.


Core Management
===============
The main actions that you can take on a *core* are the following:
- **Program**. When you load the FPGA bitstream the core's memory is empty.
By programming the core memory you overwrite any previous application.
- **enable** and **disable**. When the core is disabled its reset line is
asserted so it is not operative.
- **start** and **stop**. It starts or stops the Real-Time application
execution, so when you stop the core it will stop to run the application.
As soon as you start the execution again, the core start to execute the
application from the previous point.


Host Message Queue
==================
The main communication channel between the Host and the Real Time applications
running on the FPGA cores is the **Host Message Queue**. The HMQ allow the
Host and the Real Time application to exchange messages. For the time being
there is not a well defined protocol for the messages sent over the HMQ but as
a convention we are using the first 8 bytes as an header containing a sequence
number and a message type identifier. Then the content is up to the application.

The library allow you to send and receive asynchronous messages or to send
synchronous message, which means that you always get an answer to the sent
messages.

For synchronous messages, the driver assume that the second payload word is the
sequence number. So, if you want to use synchronous messages you have to keep in
mind this constraint.



Shared Memory
=============
The **shared memory** is mainly used to synchronize the Real-Time applications
running on different cores, but it can be accessed as well from the host system.
Basically, you can read/write any shared memory location performing different
action. These actions are defined by trtl_smem_modifier enumeration.


Debug
=====
The **debug** interface is just a very simple serial channel from the cores to
the host system (From the host point of view the debug interface is read only).
It appears under the debugfs file-system, so you must mount the debugfs before
using the debug interface; otherwise you will only get error messages.
The only purpose of the debug interface is to send messages from a Real Time
application to the host system. It's not meant to be a perfect and high
performance communication channel, so it may happen that you loose messages in
some cases (e.g. high rate messages).
