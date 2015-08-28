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

#include <string>
#include "ignition/transport/TopicUtils.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the topic names.
TEST(TopicUtilsTest, testTopics)
{
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("abc"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("abc/de"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("a"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("abc/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc/d"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc/d/e"));

  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(""));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(" "));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("~a"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("topic1 "));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("abc//def"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("ab~cd"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("/"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("~/"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("~"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("@partition"));
}

//////////////////////////////////////////////////
/// \brief Check the topic namespace.
TEST(TopicUtilsTest, testNamespaces)
{
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("abcde"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("abcde/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde/fg"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde/fg/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace(""));

  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace(" "));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("ns "));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("abc//def"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("ab~cd"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("~/abcde"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("~abcde"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("@namespace"));
}

//////////////////////////////////////////////////
/// \brief Check FullyQualifiedMsgName.
TEST(TopicUtilsTest, testFullyQualifiedMsgName)
{
  using namespace transport;
  std::string p0 = "@partition";
  std::string p1 = "@partition/@";
  std::string p2 = "@@";
  std::string p3 = "partition";
  std::string p4 = "";

  std::string ns0 = "~ns";
  std::string ns1 = "";
  std::string ns2 = "abc";
  std::string t1 = "~/def";
  std::string t2 = "~def";
  std::string t3 = "/def";
  std::string t4 = "def/";
  std::string t5 = "def/ghi";
  std::string t6 = "def/ghi/";
  std::string t7 = "~/def/";
  std::string t8 = "~def/";
  std::string name;

  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns0, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns1, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p0, ns2, t8, name));

  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns0, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns1, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p1, ns2, t8, name));

  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns0, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns1, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p2, ns2, t8, name));

  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns0, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t2, name));
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t3, name));
  EXPECT_EQ(name, "@/partition@msg@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns1, t3, name));
  EXPECT_EQ(name, "@/partition@srv@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t4, name));
  EXPECT_EQ(name, "@/partition@msg@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns1, t4, name));
  EXPECT_EQ(name, "@/partition@srv@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t5, name));
  EXPECT_EQ(name, "@/partition@msg@/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns1, t5, name));
  EXPECT_EQ(name, "@/partition@srv@/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t6, name));
  EXPECT_EQ(name, "@/partition@msg@/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns1, t6, name));
  EXPECT_EQ(name, "@/partition@srv@/def/ghi");
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns1, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t2, name));
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t3, name));
  EXPECT_EQ(name, "@/partition@msg@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns2, t3, name));
  EXPECT_EQ(name, "@/partition@srv@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t4, name));
  EXPECT_EQ(name, "@/partition@msg@/abc/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns2, t4, name));
  EXPECT_EQ(name, "@/partition@srv@/abc/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t5, name));
  EXPECT_EQ(name, "@/partition@msg@/abc/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns2, t5, name));
  EXPECT_EQ(name, "@/partition@srv@/abc/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t6, name));
  EXPECT_EQ(name, "@/partition@msg@/abc/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p3, ns2, t6, name));
  EXPECT_EQ(name, "@/partition@srv@/abc/def/ghi");
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p3, ns2, t8, name));

  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t2, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t3, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t4, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t5, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t6, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns0, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t2, name));
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t3, name));
  EXPECT_EQ(name, "@@msg@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns1, t3, name));
  EXPECT_EQ(name, "@@srv@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t4, name));
  EXPECT_EQ(name, "@@msg@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns1, t4, name));
  EXPECT_EQ(name, "@@srv@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t5, name));
  EXPECT_EQ(name, "@@msg@/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns1, t5, name));
  EXPECT_EQ(name, "@@srv@/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t6, name));
  EXPECT_EQ(name, "@@msg@/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns1, t6, name));
  EXPECT_EQ(name, "@@srv@/def/ghi");
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns1, t8, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t1, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t2, name));
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t3, name));
  EXPECT_EQ(name, "@@msg@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns2, t3, name));
  EXPECT_EQ(name, "@@srv@/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t4, name));
  EXPECT_EQ(name, "@@msg@/abc/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns2, t4, name));
  EXPECT_EQ(name, "@@srv@/abc/def");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t5, name));
  EXPECT_EQ(name, "@@msg@/abc/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns2, t5, name));
  EXPECT_EQ(name, "@@srv@/abc/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t6, name));
  EXPECT_EQ(name, "@@msg@/abc/def/ghi");
  EXPECT_TRUE(TopicUtils::FullyQualifiedSrvName(p4, ns2, t6, name));
  EXPECT_EQ(name, "@@srv@/abc/def/ghi");
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t7, name));
  EXPECT_FALSE(TopicUtils::FullyQualifiedMsgName(p4, ns2, t8, name));
}

//////////////////////////////////////////////////
/// \brief Check PartitionFromName.
/// Some of the strings are not valid fully qualified names but
/// PartitionFromName() just checks for the first two '@'.
TEST(TopicUtilsTest, testPartitionFromName)
{
  std::string n1 = "@bad";
  std::string n2 = "bad2";
  std::string n3 = "@~@";
  std::string n4 = "@partition@good";
  std::string n5 = "@@good";
  std::string n6 = "@partition@type@demi";
  std::string n7 = "@partition@msg@good";
  std::string n8 = "@partition@srv@good";
  std::string n9 = "@msg@good";

  std::string partition;

  EXPECT_FALSE(transport::TopicUtils::PartitionFromName(n1, partition));
  EXPECT_FALSE(transport::TopicUtils::PartitionFromName(n2, partition));
  EXPECT_FALSE(transport::TopicUtils::PartitionFromName(n3, partition));
  EXPECT_TRUE(transport::TopicUtils::PartitionFromName(n4, partition));
  EXPECT_EQ(partition, "partition");
  EXPECT_TRUE(transport::TopicUtils::PartitionFromName(n5, partition));
  EXPECT_EQ(partition, "");
  EXPECT_TRUE(transport::TopicUtils::PartitionFromName(n6, partition));
  EXPECT_EQ(partition, "partition");
  EXPECT_TRUE(transport::TopicUtils::PartitionFromName(n7, partition));
  EXPECT_EQ(partition, "partition");
  EXPECT_TRUE(transport::TopicUtils::PartitionFromName(n8, partition));
  EXPECT_EQ(partition, "partition");
  EXPECT_TRUE(transport::TopicUtils::PartitionFromName(n9, partition));
  EXPECT_EQ(partition, "msg");
}

//////////////////////////////////////////////////
/// \brief Check TypeFromName.
/// Some of the strings are not valid fully qualified names but
/// TypeFromName() just checks for the three '@'.
TEST(TopicUtilsTest, testTypeFromName)
{
  std::string n1 = "@bad";
  std::string n2 = "bad2";
  std::string n3 = "@partition@good";
  std::string n4 = "@@good";
  std::string n5 = "@partition@type@demi";
  std::string n6 = "@partition@msg@good";
  std::string n7 = "@partition@srv@good";
  std::string n8 = "@msg@good";

  std::string type;

  EXPECT_FALSE(transport::TopicUtils::TypeFromName(n1, type));
  EXPECT_FALSE(transport::TopicUtils::TypeFromName(n2, type));
  EXPECT_FALSE(transport::TopicUtils::TypeFromName(n3, type));
  EXPECT_FALSE(transport::TopicUtils::TypeFromName(n4, type));
  EXPECT_FALSE(transport::TopicUtils::TypeFromName(n5, type));
  EXPECT_TRUE(transport::TopicUtils::TypeFromName(n6, type));
  EXPECT_EQ(type, "msg");
  EXPECT_TRUE(transport::TopicUtils::TypeFromName(n7, type));
  EXPECT_EQ(type, "srv");
  EXPECT_FALSE(transport::TopicUtils::TypeFromName(n8, type));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
