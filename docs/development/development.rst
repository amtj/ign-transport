=========
Discovery
=========

Discovery class implements a distributed topic discovery protocol. It uses UDP
multicast for sending/receiving messages and stores updated topic information.
The discovery clients can request the discovery of a topic or the advertisement
of a local topic. The discovery uses heartbeats to track the state of other
peers in the network. The discovery nodes can register callbacks to detect
when new topics are discovered or topics are no longer available.

API
===

- **Constructor of the dicovery class:**
  The constructor has three parameters. ``_pUuid`` is the transport process
  UUID. ``_port`` is UDP port for discovery protocol traffic. ``_verbose`` is
  used for enabling verbose mode. ``_verbose`` has false as default value.

.. code-block:: cpp

    Discovery(const std::string &_pUuid,
              const int _port,
              const bool _verbose = false);

- **Discovery<MessagePublisher>**
  or equivalent ``MsgDiscovery`` type is used to create discovery object for
  messages.

.. code-block:: cpp

  // creating discovery object for message
  MsgDiscovery discovery(pUuid, msgPort);
  // or equivalent
  // Discovery<MessagePublisher> discovery(pUuid, msgPort);

- **Discovery<ServicePublisher>**
  or equivalent ``SrvDiscovery`` type is used to create discovery object for
  services.

.. code-block:: cpp

  // creating discovery object for service
  SrvDiscovery discovery(pUuid, srvPort);
  // or equivalent
  // Discovery<ServicePublisher> discovery(pUuid, srvPort);

- **Callbacks:**
  Nodes register the callbacks for receiving discovery notifications. The
  prototype of the callback contains the publisher's information advertising a
  topic. ``ConnectionsCb()`` and ``DisconnectionCb()`` register callbacks
  that will be executed when the topic address is discovered or when the node
  providing the topic is disconnected.

.. code-block:: cpp

  void onDiscoveryResponse(const MessagePublisher &_publisher);
  void onDisconnection(const MessagePublisher &_publisher);

  // the function will be executed each time a new topic is connected
  discovery.ConnectionsCb(onDiscoveryResponse);
  // the function will be executed each time a topic is no longer active
  discovery.DisconnectionsCb(onDisconnection);

- **Start the discovery protocol:**
  It is a function without parameter. Nodes can use the discovery features
  without calling ``Start()``.

.. code-block:: cpp

  discovery.Start();

More information here_.

.. _here: https://osrf-distributions.s3.amazonaws.com/ign-transport/api/0.9.0/classignition_1_1transport_1_1Discovery.html

Threading model
===============

Discovery class is using three primary threads:

- **Reception thread:**
  It is used as incharge of receiving and handling incoming messages.
  Information is updated depending on the message type. Boolean
  ``threadReceptionExiting`` is true when the reception thread is finished.

- **Heartbeat thread:**
  Each node multicasts periodic heartbeats to keep its topic information
  alive in other nodes. Heartbeat message is sent after heartbeat interval
  (in milliseconds). It also re-advertise topics. Boolean
  ``threadHeartbeatExiting`` is true when the hearbeat thread is finished.

- **Activity thread:**
  Discovery checks validity of the topic information after every activity
  interval (in milliseconds). Each node stores activity information and this
  information is updated every time new message is received. Timestamp for each
  process UUID is stored and activity information is discarded after silence
  interval (in milliseconds). Boolean ``threadActivityExiting`` is true when
  the activity thread is finished.

Boolean exit is used to mark finishing of the service threads. When true, the
service threads will finish.


Wire Protocol
=============

**Discovery header format:**
::
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            Version            |     Process UUID Length       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Process UUID Length                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Process UUID Length       |         Process UUID          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Process UUID                        |
  \                                                               \
  \                                                               \
  |                           Process UUID                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |         Process UUID          |  Message Type |     Flags     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Flags     |
  +-+-+-+-+-+-+-+-+

Note that each tick mark represents one bit position.

Version: 16 bits
  Version field indicates format of the discovery header. This document
  describes version 4.

Process UUID Length: 64 bits
  Process UUID Length is the length of the process UUID.

Process UUID: 288 bits
  Process UUID is the unique source address.

Message Type: 8 bits
  The Message Type provides an indication how to process message. There are
  eight message types:
::

  0 - Uninitialized
  1 - AdvType
  2 - SubType
  3 - UnadvType
  4 - HeartbeatType
  5 - ByeType
  6 - NewConnection
  7 - EndConnection

0. **Uninitialized:**
   This is the default value for message type. Nodes cannot send a message with
   this type through the network.

1. **AdvType:**
   Advertise message is used to multicast information about the node
   advertising a topic. It contains information about the publisher. It looks
   like::

     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | Topic Length  |     Topic     | Zmq Addr Len  |    Zmq Addr   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | pUUID Length  |     pUUID     | nUUID Length  |      nUUID    |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  Topic Scope  |
     +-+-+-+-+-+-+-+-+

   Also it has an additional information depends on type of publisher. Service
   publisher has::

     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | Socket ID Len |   Socket ID   |  req Type Len |    req Type   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  res Type Len |    res Type   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Message publisher has::

     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  zmq Ctrl Len |    zmq Ctrl   |  Msg Type Len | Msg Type Name |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

2. **SubType:**
   Subscription message is used for requesting information about a given topic::

     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | Topic Length  |     Topic     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

3. **UnadvType:**
   Unadvertise message is used to multicast a discovery message that will cancel
   all the discovery information for the topic advertised by a specific node.
   It contains the same information as **AdvType** message.

4. **HeartbeatType:**
   Heartbeat message is used to multicast periodic heartbeats to keep node's
   topic information alive in other nodes. It contains only header.

5. **ByeType:**
   Bye message is used to multicast messages to trigger the remote cancellation
   of all node's advertised topics. It contains only header.

6. **NewConnection:**
   It is used to mark a new connection.

7. **EndConnection:**
   It is used to mark end of a connection.

Flags: 16 bits
  It is used for optional flags.

Network Interfaces
==================

The protocol can automatically provide settings for network interfaces. First
of all, it collects information about the network interfaces. Then it selects
the main host. It checks the ip address which is connected with hostname. If
the address is not a public ip or a localhost, then this address is used.
Otherwise, arbitrary network interface which has a public ip address is used.
Finally, if there is no public ip address then the private ip address is used.
If the computer does not have any network interface except ``127.0.0.1``,
then ip address ``127.0.0.1`` will be used. This should work for local
processes, but will seldomly work for remote processes.

Every network interface is associated with its own unique socket. Discovery
protocol maintains a list of sockets associated with each network interface.
Every time discovery sends information into the wire, the information is sent
through all the sockets. This is to guarantee that the discovery information
reaches every interface on the network. Nodes will receive discovery information
from one socket only. This socket is associated with the main host address.

Discovery protocol uses a UDP multicast for sending and receiving messages.
Every network interface needs to register in the multicast group.

If the environmental variable ``IGN_IP`` is set, then the protocol will use
this ip address for sending and receiving messages. If environment variable
``IGN_IP`` looks like invalid ip, then ip address ``127.0.0.1`` will be used.
It will also throw a warning.
