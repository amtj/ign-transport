/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#include "ignition/transport/SubscribeOptions.hh"
#include "ignition/transport/SubscribeOptionsPrivate.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
SubscribeOptions::SubscribeOptions()
  : dataPtr(new SubscribeOptionsPrivate())
{
}

//////////////////////////////////////////////////
SubscribeOptions::~SubscribeOptions()
{
}

//////////////////////////////////////////////////
int SubscribeOptions::SetMsgsPerSec()
{
  return this->dataPtr->MsgsPerSec;
}

//////////////////////////////////////////////////
void SubscribeOptions::SetMsgsPerSec(int _NewMsgsPerSec)
{
  this->dataPtr->MsgsPerSec = _NewMsgsPerSec;
}
