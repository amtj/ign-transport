/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <chrono>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "ignition/transport/test_config.h"
#include "msg/int.pb.h"

using namespace ignition;

bool cbExecuted;
std::string topic = "/foo";
int data = 5;

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const transport::msgs::Int &_msg)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
}

//////////////////////////////////////////////////
void subscriber()
{
  cbExecuted = false;
  transport::Node node;

  EXPECT_TRUE(node.Subscribe(topic, cb));

  int i = 0;
  while (i < 100 && !cbExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the message was not received because the scope was Process.
  EXPECT_FALSE(cbExecuted);
  cbExecuted = false;
}

TEST(ScopedTopicTest, SubscriberTest)
{
  subscriber();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Partition name has not be passed as argument" << std::endl;
    return -1;
  }

  // Set the partition name for this test.
  setenv("IGN_PARTITION", argv[1], 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
