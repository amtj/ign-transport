===========
Development
===========

Discovery
=========

A discovery class implements a distributed topic discovery protocol. It uses UDP
broadcast for sending/receiving messages and stores  updated topic information.
The discovery clients can request the discovery of a topic or the advertisement
of a local topic. The discovery uses heartbeats to track the state of other
peers in the network. The discovery clients can register callbacks to detect
when new topics are discovered or topics are no longer available.

Threading model
===============

Discovery class is using three main threads:

    1. ``threadReception`` is used as incharge of receiving and handling incoming
       messages. Boolean ``threadReceptionExiting`` is true when the reception
       thread is finishing.

    2. ``threadHeartbeat`` is used as incharge of sending heartbeats. Boolean
       ``threadHeartbeatExiting`` is true when the hearbeat thread is finishing.

    3. ``threadActivity`` is used as incharge of update the activity. Boolean
       ``threadActivityExiting`` is true when the activity thread is finishing.

Boolean exit is used to mark finishing of the service threads. When true, the
service threads will finish.


Wire protocol
=============

Discovery mainly broadcasts three types of messages.

    1. [un]Advertise messages: This is the case for advertise, unadvertise,
       advertise service and unadvertise service messages. The message is
       contained in the feilds ``advMsg(header, _pub)`` and it has size of
       ``advMsg.MsgLength()``.

    2. Subscribe messages: This is the case for subscribe and subscribe service
       messages. The message is contained in the feilds ``subMsg(header, topic)``
       and it has size of ``subMsg.MsgLength()``.

    3. Heartbeat and Bye messages: This is the case for heartbyte and bye
       messages.The message is contained in the ``header`` feild and it has size of
       ``header.HeaderLength()``.


Network interfaces
==================

Boolean ``RegisterNetIface(const std::string &_ip)`` is used to register a new
network interface in the discovery system. Here ``_ip`` is a IP address to register.
Boolean is true when the interface was successfully registered or false
otherwise (e.g.: invalid IP address). Even if a machine have several network
interfaces (e.g.: wired, wireless, and localhost.), every IP address should be
registered in the same manner.
