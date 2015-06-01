Library Overview {#mainpage}
================
This is the **WRTD** library documentation. Here you can find all
the information about the *White-Rabbit Trigger-Distribution* API and the main
library behavior that you need to be aware of.

If you are reading this from the doxygen documentation, then you can find
the API documentation in the usual Doxygen places. Otherwise, you can get
the API documentation directly from the source code that you can find in
the *lib* directory.

In this document we are going to provides you some clues to understand how
to use the library API.

This library is completely base on the *White-Rabbit Node-Core* library.
It uses all its features to establish the communication between the
Trigger Distribution Real Time applications. This library hides the knowledge
about the conventions (protocol) used between the Host and the Real Time
applications and it exposes a simple API for the interaction.

While reading any documentation we suggest you to read the dictionary to avoid
misinterpretation.


Overview
========
The White-Rabbit Trigger-Distribution is a system that allow its users to detect
a trigger (pulse), propagate it over the White-Rabbit network and reproduce it
on a remote machine. This library is in charge to ease the configuration of this
system. The system is based on the White-Rabbit Node-Core. There are two cores
running separately two Real Time applications: one to manage the input,
one to manage the output.

              +--------------Trigger-Distribution-System-------------+
              | RealTime App     - - - - - - - - -      RealTime App |
    -\        |  +-------+     (                   )     +--------+  |        /-
     |--------|->| INPUT |--->( WhiteRabbit Network )--->| OUTPUT |--|------->|
    -/  PULSE |  +-------+     (                   )     +--------+  | PULSE  \-
              |                  - - - - - - - - -                   |
              +------------------------------------------------------+


Initialization
==============
To be able to use this library the first thing to do is to initialize a library
instance using wrtd_init(); form this point on you are able to use the
library API. Of course, when you finished to use the library you have to
remove this instance using wrtd_exit().

At the beginning, all communication channels are close, so before start to
communicate with the Real Time application you have to open your communication
channel. Then, close it when you have done.


Logging
=======
The WRTD Real Time applications are able to provide some logging information
about the things happening on the FPGA. This interface is read only, so
you can only read logging messages. There are only two configuration parameters:
one for the *logging level*; the other one to set the exclusivity access to the
logging interface. When the logging interface is shared, then also other users
are allowed to get the same messages.


Input
=====
The *input* Real Time application and the associated part of library manages
the detection of the incoming pulse and their correct propagation over the
white-rabbit network as trigger event.
The library allows the user to fully configure the input parameter and to get
the current status of Real Time application but also to get information about
the triggers associated to the input channels. Go directly to the API for the
list of parameters that you can set.


Output
======
The *input* Real Time application and the associated part of library manages
the pluse generation and their correct reception from the white-rabbit
network as trigger event.
The library allows the user to fully configure the output parameter and to get
the current status of Real Time application but also to get information about
the trigger associated to the output channesl. Go directly to the API for the
list of parameters that you can set.
