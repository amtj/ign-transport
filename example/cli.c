/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#include <ignition/transport/clib.hh>

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  int param1 = 1;
  const char *param2 = "test string";

  void *f_ign_test = FUNC_MACRO(ign_test_int)();

  //ign_test(param1);
  //ign_test(param2);

  return 0;
}
