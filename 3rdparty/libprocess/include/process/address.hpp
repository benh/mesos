// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#ifndef __PROCESS_ADDRESS_HPP__
#define __PROCESS_ADDRESS_HPP__

#include <stdint.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif // __WINDOWS__

#ifndef __WINDOWS__
#include <arpa/inet.h>
#endif // __WINDOWS__

#include <glog/logging.h>

#ifndef __WINDOWS__
#include <sys/un.h>
#endif // __WINDOWS__

#include <ostream>

#include <boost/functional/hash.hpp>

#include <stout/abort.hpp>
#include <stout/check.hpp>
#include <stout/ip.hpp>
#include <stout/net.hpp>
#include <stout/stringify.hpp>

namespace process {
namespace network {

namespace unix {
class Address;
} // namespace unix {

namespace inet {
class Address;
} // namespace inet {

// Represents a network "address", subsuming the `struct addrinfo` and
// `struct sockaddr` that typically is used to encapsulate an address.
//
// TODO(jieyu): Move this class to stout.
class Address
{
public:
  enum class Family {
#ifndef __WINDOWS__
    UNIX,
#endif // __WINDOWS__
    INET
  };

  static Try<Address> create(const sockaddr_storage& storage)
  {
    switch (storage.ss_family) {
      case AF_UNIX:
      case AF_INET:
        return Address(storage);
      default:
        return Error(
            "Unsupported family type: " + stringify(storage.ss_family));
    }
  }

  Family family() const
  {
    if (storage.ss_family == AF_UNIX) {
      return Family::UNIX;
    } else if (storage.ss_family == AF_INET) {
      return Family::INET;
    } else {
      ABORT("Unexpected family type: " + stringify(storage.ss_family));
    }
  }

  // Returns the storage size depending on the family of this address.
  size_t size() const
  {
    switch (family()) {
#ifndef __WINDOWS__
      case Family::UNIX:
        return sizeof(sockaddr_un);
#endif // __WINDOWS__
      case Family::INET:
        return sizeof(sockaddr_in);
    }
  }

  operator sockaddr_storage() const
  {
    return storage;
  }

private:
  friend class unix::Address;
  friend class inet::Address;
  friend std::ostream& operator<<(std::ostream& stream, const Address& address);

  Address(const sockaddr_storage& _storage) : storage(_storage) {}

  sockaddr_storage storage;
};


#ifndef __WINDOWS__
namespace unix {

class Address
{
public:
  Address(const std::string& _path) : path(_path) {}

  operator network::Address() const
  {
    sockaddr_storage storage;
    memset(&storage, 0, sizeof(storage));
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), path.length());
    memcpy(&storage, &addr, sizeof(addr));
    return network::Address(storage);
  }

  std::string path;
};


inline std::ostream& operator<<(
    std::ostream& stream,
    const Address& address)
{
  // TODO(benh): Properly print out abstract paths (Linux only).
  return stream << address.path;
}

} // namespace unix {
#endif // __WINDOWS__

namespace inet {

class Address
{
public:
  Address() : ip(INADDR_ANY), port(0) {}

  Address(const net::IP& _ip, uint16_t _port) : ip(_ip), port(_port) {}

  static const Address LOOPBACK_ANY;

  static const Address ANY_ANY;

  /**
   * Returns the hostname of this address's IP.
   *
   * @returns the hostname of this address's IP.
   */
  // TODO(jmlvanre): Consider making this return a Future in order to
  // deal with slow name resolution.
  Try<std::string> hostname() const
  {
    const Try<std::string> hostname = ip == net::IP(INADDR_ANY)
      ? net::hostname()
      : net::getHostname(ip);

    if (hostname.isError()) {
      return Error(hostname.error());
    }

    return hostname.get();
  }

  bool operator<(const Address& that) const
  {
    if (ip == that.ip) {
      return port < that.port;
    } else {
      return ip < that.ip;
    }
  }

  bool operator>(const Address& that) const
  {
    if (ip == that.ip) {
      return port > that.port;
    } else {
      return ip > that.ip;
    }
  }

  bool operator==(const Address& that) const
  {
    return (ip == that.ip && port == that.port);
  }

  bool operator!=(const Address& that) const
  {
    return !(*this == that);
  }

  operator network::Address() const
  {
    sockaddr_storage storage;
    memset(&storage, 0, sizeof(storage));
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = ip.in().get();
    addr.sin_port = htons(port);
    memcpy(&storage, &addr, sizeof(addr));
    return network::Address(storage);
  }

  net::IP ip;
  uint16_t port;
};


inline std::ostream& operator<<(std::ostream& stream, const Address& address)
{
  stream << address.ip << ":" << address.port;
  return stream;
}

} // namespace inet {


template <typename AddressType>
Try<AddressType> convert(Try<Address>&& address);


template <>
inline Try<Address> convert(Try<Address>&& address)
{
  return address;
}


#ifndef __WINDOWS__
template <>
inline Try<unix::Address> convert(Try<Address>&& address)
{
  if (address.isError()) {
    return Error(address.error());
  }

  if (address->family() == Address::Family::UNIX) {
    sockaddr_storage storage = address.get();
    CHECK(storage.ss_family == AF_UNIX);
    sockaddr_un* addr = (sockaddr_un*) &storage;
    return unix::Address(std::string(addr->sun_path));
  }

  return Error("Unexpected address family");
}
#endif // __WINDOWS__


template <>
inline Try<inet::Address> convert(Try<Address>&& address)
{
  if (address.isError()) {
    return Error(address.error());
  }

  if (address->family() == Address::Family::INET) {
    sockaddr_storage storage = address.get();
    CHECK(storage.ss_family == AF_INET);
    sockaddr_in* addr = (sockaddr_in*) &storage;
    return inet::Address(net::IP(addr->sin_addr), ntohs(addr->sin_port));
  }

  return Error("Unexpected address family");
}


inline std::ostream& operator<<(std::ostream& stream, const Address& address)
{
  switch (address.family()) {
#ifndef __WINDOWS__
    case Address::Family::UNIX: {
      sockaddr_un* addr = (sockaddr_un*) &address.storage;
      return stream << unix::Address(std::string(addr->sun_path));
    }
#endif // __WINDOWS__
    case Address::Family::INET: {
      sockaddr_in* addr = (sockaddr_in*) &address.storage;
      return stream << inet::Address(
          net::IP(addr->sin_addr),
          ntohs(addr->sin_port));
    }
  }
}

} // namespace network {
} // namespace process {

namespace std {

template <>
struct hash<process::network::inet::Address>
{
  typedef size_t result_type;

  typedef process::network::inet::Address argument_type;

  result_type operator()(const argument_type& address) const
  {
    size_t seed = 0;
    boost::hash_combine(seed, std::hash<net::IP>()(address.ip));
    boost::hash_combine(seed, address.port);
    return seed;
  }
};

} // namespace std {

#endif // __PROCESS_ADDRESS_HPP__
