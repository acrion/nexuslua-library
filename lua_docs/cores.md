cores                         {#cores}
========

The nexuslua function [cores](cores.md) returns an estimation of the number of cores the system supports. This estimation is based on https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency and includes virtual cores.

It is meant to be used to set the value of the `threads` entry in nexuslua message to a useful value. For an example, see [send](send.md).