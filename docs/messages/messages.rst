================================
Node communication via messages
================================

In this tutorial, we are going to create two nodes that are going to communicate
via messages. One node will be a publisher that generates the information,
whereas the other node will be the subscriber consuming the information. Our
nodes will be running on different processes within the same machine.

.. code-block:: bash

    mkdir ~/ign_transport_tutorial
    cd ~/ign_transport_tutorial
    mkdir src

Creating the publisher
======================

Create the *src/publisher.cc* file within the *ign_transport_tutorial* and paste
the following code inside it:

.. code-block:: cpp

    #include <chrono>
    #include <string>
    #include <thread>
    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

    using namespace ignition;

    bool terminatePub = false;

    //////////////////////////////////////////////////
    /// \brief Function callback executed when a SIGINT or SIGTERM signals are
    /// captured. This is used to break the infinite loop that publishes
    /// messages and exit the program smoothly.
    void signal_handler(int _signal)
    {
      if (_signal == SIGINT || _signal == SIGTERM)
        terminatePub = true;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      std::string topic = "topicA";
      std::string data = "helloWorld";

      // Create a transport node.
      transport::Node node;

      // Advertise a topic.
      publisher.Advertise<tutorial::msgs::StringMsg>(topic);

      // Prepare the message.
      tutorial::msgs::StringMsg msg;
      msg.set_data(data);

      // Publish messages at 1Hz.
      while (!terminatePub)
      {
        node.Publish(topic, msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }

Walkthrough
===========

.. code-block:: cpp

    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

The line *#include <ignition/transport/Node.hh>* contains the Ignition Transport
header for using the transport library.

The next line includes the generated protobuf code that we are going to use
for our messages. We are going to publish *StringMsg* type protobuf messages
defined in *stringmsg.pb.h*.

.. code-block:: cpp

    // Create a transport node.
    transport::Node node;

    // Advertise a topic.
    publisher.Advertise<tutorial::msgs::StringMsg>(topic);

First of all we declare a *Node* that will offer all the transport
functionality. In our case, we are interested on publishing topic updates, so
the first step is to announce our topic name and its type. Once a topic name is
advertised, we can start publishing periodic messages.

.. code-block:: cpp

    // Prepare the message.
    tutorial::msgs::StringMsg msg;
    msg.set_data(data);

    // Publish messages at 1Hz.
    while (!terminatePub)
    {
      node.Publish(topic, msg);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

In this section of the code we create a protobuf message and fill it with
content. Next, we iterate in a loop that publishes one message every second.
The method *Publish()* sends a message to all the subscribers.

Creating the subscriber
=======================

Create the *src/subscriber.cc* file within the *ign_transport_tutorial* and
paste the following code inside it:

.. code-block:: cpp

    #include <cstdio>
    #include <iostream>
    #include <string>
    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

    //////////////////////////////////////////////////
    /// \brief Function called each time a topic update is received.
    void cb(const example::msgs::StringMsg &_msg)
    {
      std::cout << "Msg: " << _msg.data() << std::endl << std::endl;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      ignition::transport::Node node;
      std::string topic = "/foo";

      // Subscribe to a topic by registering a callback.
      if (!node.Subscribe(topic, cb))
      {
        std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
        return -1;
      }

      // Zzzzzz.
      std::cout << "Press <ENTER> to exit" << std::endl;
      getchar();

      return 0;
    }


Walkthrough
===========

.. code-block:: cpp

    //////////////////////////////////////////////////
    /// \brief Function called each time a topic update is received.
    void cb(const example::msgs::StringMsg &_msg)
    {
      std::cout << "Msg: " << _msg.data() << std::endl << std::endl;
    }

We need to register a function callback that will execute every time we receive
a new topic update. The signature of the callback is always similar to the one
shown in this example with the only exception of the protobuf message type.
You should create a function callback with the appropriate protobuf type
depending on the type of the topic advertised. In our case, we know that topic
*/topicA* will contain a protobuf *StringMsg* type.

.. code-block:: cpp

    ignition::transport::Node node;
    std::string topic = "/foo";

    // Subscribe to a topic by registering a callback.
    if (!node.Subscribe(topic, cb))
    {
      std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
      return -1;
    }

After the node creation, the method *Subscribe()* allows you to subscribe to a
given topic name by specifying your subscription callback function.


Building the code
=================

Copy this *CMakeLists.txt* file within the *ign_transport_tutorial*. This is the
top level cmake file that will check for dependencies.

Copy this *stringmsg.proto* file within the *ign_transport_tutorial/src*.
This file contains the Protobuf message definition that we use in this example.

Copy this *CMakeLists.txt* file within the *ign_transport_tutorial/src*. This is
the cmake file that will generate the C++ code from the Protobuf file and will
create the *publisher* and *subscriber* executables.

Once you have all your files, go ahead and create a *build/* directory within
the *ign_transport_tutorial* directory.

.. code-block:: bash

    mkdir build
    cd build

Run *cmake* and build the code.

.. code-block:: bash

    cmake ..
    make


Running the examples
====================

Open two new terminals and from your *build/* directory run the executables:

From terminal 1

.. code-block:: bash

    ./publisher

From terminal 2

.. code-block:: bash

    ./subscriber


In your subscriber terminal, you should expect an output similar to this one,
showing that your subscribing is receiving the topic updates:

.. code-block:: bash

    caguero@turtlebot:~/ign_transport_tutorial/build$ ./subscriber
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]