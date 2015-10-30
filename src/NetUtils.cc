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

#ifdef _WIN32
  #include <Winsock2.h>
  #include <iphlpapi.h>
#else
  #include <arpa/inet.h>
  #include <net/if.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

#include <stdlib.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "ignition/transport/config.hh"
#include "ignition/transport/NetUtils.hh"

#ifdef HAVE_IFADDRS
# include <ifaddrs.h>
#endif

using namespace ignition;

//////////////////////////////////////////////////
bool transport::isPrivateIP(const char *_ip)
{
  bool b = !strncmp("192.168", _ip, 7) || !strncmp("10.", _ip, 3) ||
           !strncmp("169.254", _ip, 7);
  return b;
}

//////////////////////////////////////////////////
int transport::hostnameToIp(char *_hostname, std::string &_ip)
{
  struct hostent *he;
  struct in_addr **addr_list;
  int i;

  if ((he = gethostbyname(_hostname)) == nullptr)
  {
#ifndef _WIN32
    // Complain, but not on Windows, where this apparently always happens.
    std::cerr << "Error in gethostbyname when using hostname = " << _hostname
      << std::endl;
#endif
    return 1;
  }

  addr_list = (struct in_addr **) he->h_addr_list;

  for (i = 0; addr_list[i] != nullptr; ++i)
  {
    // Return the first one;
    _ip = std::string(inet_ntoa(*addr_list[i]));
    return 0;
  }

  return 1;
}

//////////////////////////////////////////////////
std::string transport::determineHost()
{
  char *ip_env;
  // First, did the user set IGN_IP?
  ip_env = std::getenv("IGN_IP");

  if (ip_env)
  {
    if (strlen(ip_env) != 0)
      return ip_env;
    else
      std::cerr << "invalid IGN_IP (an empty string)" << std::endl;
  }

  // Second, try the hostname
  char host[1024];
  memset(host, 0, sizeof(host));
  if (gethostname(host, sizeof(host) - 1) != 0)
    std::cerr << "determineIP: gethostname failed" << std::endl;

  // We don't want localhost to be our ip
  else if (strlen(host) && strcmp("localhost", host))
  {
    std::string hostIP;
    strcat(host, ".local");
    if (hostnameToIp(host, hostIP) == 0 && !isPrivateIP(hostIP.c_str()))
    {
      return hostIP;
    }
  }

  // Third, fall back on interface search, which will yield an IP address
#ifdef HAVE_IFADDRS
  struct ifaddrs *ifa = nullptr, *ifp = NULL;
  int rc;
  if ((rc = getifaddrs(&ifp)) < 0)
  {
    std::cerr << "error in getifaddrs: " << strerror(rc) << std::endl;
    exit(-1);
  }
  char preferred_ip[200] = {0};
  for (ifa = ifp; ifa; ifa = ifa->ifa_next)
  {
    char ip_[200];
    socklen_t salen;
    if (!ifa->ifa_addr)
      continue;  // evidently this interface has no ip address
    if (ifa->ifa_addr->sa_family == AF_INET)
      salen = sizeof(struct sockaddr_in);
    else if (ifa->ifa_addr->sa_family == AF_INET6)
      salen = sizeof(struct sockaddr_in6);
    else
      continue;
    if (getnameinfo(ifa->ifa_addr, salen, ip_, sizeof(ip_), nullptr, 0,
                    NI_NUMERICHOST) < 0)
    {
      std::cout << "getnameinfo couldn't get the ip of interface " <<
                   ifa->ifa_name << std::endl;
      continue;
    }
    // prefer non-private IPs over private IPs
    if (!strcmp("127.0.0.1", ip_) || strchr(ip_, ':'))
      continue;  // ignore loopback unless we have no other choice
    // Does not support multicast.
    if (!(ifa->ifa_flags & IFF_MULTICAST))
      continue;
    // Is not running.
    if (!(ifa->ifa_flags & IFF_UP))
      continue;
    if (ifa->ifa_addr->sa_family == AF_INET6 && !preferred_ip[0])
      strcpy(preferred_ip, ip_);
    else if (isPrivateIP(ip_) && !preferred_ip[0])
      strcpy(preferred_ip, ip_);
    else if (!isPrivateIP(ip_) &&
             (isPrivateIP(preferred_ip) || !preferred_ip[0]))
      strcpy(preferred_ip, ip_);
  }
  freeifaddrs(ifp);
  if (!preferred_ip[0])
  {
    std::cerr <<
      "Couldn't find a preferred IP via the getifaddrs() call; "
      "I'm assuming that your IP "
      "address is 127.0.0.1.  This should work for local processes, "
      "but will almost certainly not work if you have remote processes."
      "Report to the disc-zmq development team to seek a fix." << std::endl;
    return std::string("127.0.0.1");
  }
  return std::string(preferred_ip);
#elif defined(_WIN32)
  // Establish our default return value, in case everything below fails.
  std::string ret_addr("127.0.0.1");
  // Look up our address.
  ULONG outBufLen = 0;
  PIP_ADAPTER_ADDRESSES addrs = NULL;
  // Not sure whether these are the right flags, but they work for
  // me on Windows 7
  ULONG flags = (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                 GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME);
  // The first time, it'll fail; we're asking how much space is needed to
  // store the result.
  GetAdaptersAddresses(AF_INET, flags, NULL, addrs, &outBufLen);
  // Allocate the required space.
  addrs = new IP_ADAPTER_ADDRESSES[outBufLen];
  ULONG ret;
  // Now the call should succeed.
  if ((ret = GetAdaptersAddresses(AF_INET, flags, NULL, addrs, &outBufLen)) ==
    NO_ERROR)
  {
    // Iterate over all returned adapters, arbitrarily sticking with the
    // last non-loopback one that we find.
    for (PIP_ADAPTER_ADDRESSES curr = addrs; curr; curr = curr->Next)
    {
      // This adapter does not support multicast.
      if (curr->Flags & IP_ADAPTER_NO_MULTICAST)
        continue;

      // The interface is not running.
      if (curr->OperStatus != IfOperStatusUp)
        continue;

      // Iterate over all unicast addresses for this adapter
      for (PIP_ADAPTER_UNICAST_ADDRESS unicast = curr->FirstUnicastAddress;
           unicast; unicast = unicast->Next)
      {
        // Cast to get an IPv4 numeric address (the AF_INET flag used above
        // ensures that we're only going to get IPv4 address here).
        sockaddr_in* sockaddress =
          reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
        // Make it a dotted quad
        char ipv4_str[3*4+3+1];
        sprintf(ipv4_str, "%d.%d.%d.%d",
          sockaddress->sin_addr.S_un.S_un_b.s_b1,
          sockaddress->sin_addr.S_un.S_un_b.s_b2,
          sockaddress->sin_addr.S_un.S_un_b.s_b3,
          sockaddress->sin_addr.S_un.S_un_b.s_b4);
        // Ignore loopback address (that's our default anyway)
        if (!strcmp(ipv4_str, "127.0.0.1"))
          continue;
        ret_addr = ipv4_str;
      }
    }
  }
  else
    std::cerr << "GetAdaptersAddresses() failed: " << ret << std::endl;
  delete [] addrs;
  std::cerr << "DEBUG: Determined my IP address to be: " <<
    ret_addr << std::endl;
  if (ret_addr == "127.0.0.1")
  {
    std::cerr <<
      "Couldn't find a preferred IP via the GetAdaptersAddresses() call; "
      "I'm assuming that your IP "
      "address is 127.0.0.1.  This should work for local processes, "
      "but will almost certainly not work if you have remote processes."
      "Report to the disc-zmq development team to seek a fix." << std::endl;
  }
  return ret_addr;
#else
  // @todo Fix IP determination in the case where getifaddrs() isn't
  // available.
  std::cerr <<
    "You don't have the getifaddrs() call; I'm assuming that your IP "
    "address is 127.0.0.1.  This should work for local processes, "
    "but will almost certainly not work if you have remote processes."
    "Report to the disc-zmq development team to seek a fix." << std::endl;
  return std::string("127.0.0.1");
#endif
}

//////////////////////////////////////////////////
std::vector<std::string> transport::determineInterfaces()
{
  char *ip_env;
  std::vector<std::string> result;
  // First, did the user set IGN_IP?
  ip_env = std::getenv("IGN_IP");

  if (ip_env)
  {
    if (strlen(ip_env) != 0)
    {
      result.push_back(std::string(ip_env));
      return result;
    }
    else
      std::cerr << "invalid IGN_IP (an empty string)" << std::endl;
  }

  // Second, fall back on interface search, which will yield an IP address
#ifdef HAVE_IFADDRS
  struct ifaddrs *ifa = nullptr, *ifp = NULL;
  int rc;
  if ((rc = getifaddrs(&ifp)) < 0)
  {
    std::cerr << "error in getifaddrs: " << strerror(rc) << std::endl;
    exit(-1);
  }
  char preferred_ip[200] = {0};

  for (ifa = ifp; ifa; ifa = ifa->ifa_next)
  {
    char ip_[200];
    socklen_t salen;
    std::string interface;
    if (!ifa->ifa_addr)
      continue;  // evidently this interface has no ip address
    if (ifa->ifa_addr->sa_family == AF_INET)
      salen = sizeof(struct sockaddr_in);
    else if (ifa->ifa_addr->sa_family == AF_INET6)
      salen = sizeof(struct sockaddr_in6);
    // Unknown family.
    else
      continue;
    if (getnameinfo(ifa->ifa_addr, salen, ip_, sizeof(ip_), nullptr, 0,
                    NI_NUMERICHOST) < 0)
    {
      std::cout << "getnameinfo couldn't get the ip of interface " <<
                   ifa->ifa_name << std::endl;
      continue;
    }
    // prefer non-loopback IPs
    if (!strcmp("127.0.0.1", ip_) || strchr(ip_, ':'))
      continue;  // ignore loopback unless we have no other choice
    // Does not support multicast.
    if (!(ifa->ifa_flags & IFF_MULTICAST))
      continue;
    // Is not running.
    if (!(ifa->ifa_flags & IFF_UP))
      continue;
    // IPv6 interface.
    if (ifa->ifa_addr->sa_family == AF_INET6 && !preferred_ip[0])
      interface = std::string(ip_);
    // Private network interface.
    else if (isPrivateIP(ip_) && !preferred_ip[0])
      interface = std::string(ip_);
    // Any other interface.
    else if (!isPrivateIP(ip_) &&
             (isPrivateIP(preferred_ip) || !preferred_ip[0]))
      interface = std::string(ip_);

    // Add the new interface if it's new and unique.
    if (!interface.empty() &&
        std::find(std::begin(result), std::end(result),
          interface) == std::end(result))
    {
      result.push_back(interface);
    }
  }
  freeifaddrs(ifp);
  if (result.empty())
  {
    std::cerr <<
      "Couldn't find a preferred IP via the getifaddrs() call; "
      "I'm assuming that your IP "
      "address is 127.0.0.1.  This should work for local processes, "
      "but will almost certainly not work if you have remote processes."
      "Report to the disc-zmq development team to seek a fix." << std::endl;
    return {"127.0.0.1"};
  }
  return result;
#elif defined(_WIN32)
  // Establish our default return value, in case everything below fails.
  std::string ret_addr("127.0.0.1");
  // Look up our address.
  ULONG outBufLen = 0;
  PIP_ADAPTER_ADDRESSES addrs = NULL;
  // Not sure whether these are the right flags, but they work for
  // me on Windows 7
  ULONG flags = (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                 GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME);
  // The first time, it'll fail; we're asking how much space is needed to
  // store the result.
  GetAdaptersAddresses(AF_INET, flags, NULL, addrs, &outBufLen);
  // Allocate the required space.
  addrs = new IP_ADAPTER_ADDRESSES[outBufLen];
  ULONG ret;
  // Now the call should succeed.
  if ((ret = GetAdaptersAddresses(AF_INET, flags, NULL, addrs, &outBufLen)) ==
    NO_ERROR)
  {
    // Iterate over all returned adapters, arbitrarily sticking with the
    // last non-loopback one that we find.
    for (PIP_ADAPTER_ADDRESSES curr = addrs; curr; curr = curr->Next)
    {
      // This adapter does not support multicast.
      if (curr->Flags & IP_ADAPTER_NO_MULTICAST)
        continue;

      // The interface is not running.
      if (curr->OperStatus != IfOperStatusUp)
        continue;

      // Iterate over all unicast addresses for this adapter
      for (PIP_ADAPTER_UNICAST_ADDRESS unicast = curr->FirstUnicastAddress;
           unicast; unicast = unicast->Next)
      {
        // Cast to get an IPv4 numeric address (the AF_INET flag used above
        // ensures that we're only going to get IPv4 address here).
        sockaddr_in* sockaddress =
          reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
        // Make it a dotted quad
        char ipv4_str[3*4+3+1];
        sprintf(ipv4_str, "%d.%d.%d.%d",
          sockaddress->sin_addr.S_un.S_un_b.s_b1,
          sockaddress->sin_addr.S_un.S_un_b.s_b2,
          sockaddress->sin_addr.S_un.S_un_b.s_b3,
          sockaddress->sin_addr.S_un.S_un_b.s_b4);
        // Ignore loopback address (that's our default anyway)
        if (!strcmp(ipv4_str, "127.0.0.1"))
          continue;
        ret_addr = ipv4_str;

        // Add the new interface if it's new and unique.
        if (!ret_addr.empty() &&
        std::find(std::begin(result),
                  std::end(result), ret_addr) == std::end(result))
        {
          result.push_back(std::string(ret_addr));
        }
      }
    }
  }
  else
    std::cerr << "GetAdaptersAddresses() failed: " << ret << std::endl;
  delete [] addrs;
  std::cerr << "DEBUG: Determined my IP address to be: " <<
    ret_addr << std::endl;
  if (result.empty() || (result.size() == 1 && result.at(0) == "127.0.0.1"))
  {
    std::cerr <<
      "Couldn't find a preferred IP via the GetAdaptersAddresses() call; "
      "I'm assuming that your IP "
      "address is 127.0.0.1.  This should work for local processes, "
      "but will almost certainly not work if you have remote processes."
      "Report to the disc-zmq development team to seek a fix." << std::endl;
  }
  return result;
#else
  // @todo Fix IP determination in the case where getifaddrs() isn't
  // available.
  std::cerr <<
    "You don't have the getifaddrs() call; I'm assuming that your IP "
    "address is 127.0.0.1.  This should work for local processes, "
    "but will almost certainly not work if you have remote processes."
    "Report to the disc-zmq development team to seek a fix." << std::endl;
  return {"127.0.0.1"};
#endif
}

#include <sys/utsname.h>
//////////////////////////////////////////////////
std::string transport::hostName()
{
  char hostname[128];
  gethostname(hostname, sizeof hostname);
  return hostname;

  struct utsname unameData;
  uname(&unameData);
  return unameData.nodename;
}
