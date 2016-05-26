==========
NodeShared
==========

Node is a class that allows a client to communicate with other peers. There are
two main communication modes: pub/sub messages and service calls. NodeShared
holds private data for Node class and this class is not supposed to be used
directly.

Threading model
===============

There is only one major thread, ``threadReception``. Reception thread is a
service thread which is in charge of receiving and handling incoming messages.
A boolean is used to mark exit of the thread. It is true when the reception
thread is finishing.

Mutex is used to ensure exclusive access between all threads.
