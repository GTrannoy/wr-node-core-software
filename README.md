Mock Turtle
===========
The project provide the software necessary to access Mock Turtle
infrastructure on an FPGA.

The software stack for the Mock Turtle is made of:
- a driver that allow you to communication with the Mock Turtle CPUs over
  the Host Message Queue mechanism. It exports also a debug interface.
- one library that ease the driver access from user-space
- a Python class that wrap the C library
- a set of command line tools to perform basic operations on the Mock Turtle
  components