// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __STOUT_IP46_HPP__
#define __STOUT_IP46_HPP__

#include <stout/ip.hpp>

namespace net {

namespace internal {
class IP : protected net::IP
{
public:
  using net::IP::isLoopback;
  using net::IP::isAny;

  // NOTE: We cannot directly inherit these operators from the base
  // class since the inheritance is "protected". Implicit typecasting
  // to the base class is therefore not allowed.
  bool operator==(const IP& that) const
  {
    return (net::IP)*this == (net::IP)that;
  }

  bool operator!=(const IP& that) const
  {
    return (net::IP)*this != (net::IP)that;
  }

  bool operator<(const IP& that) const
  {
    return (net::IP)*this < (net::IP)that;
  }

  bool operator>(const IP& that) const
  {
    return (net::IP)*this > (net::IP)that;
  }

protected:
  using net::IP::IP;
  using net::IP::in;
  using net::IP::in6;
};

} // namespace internal {

namespace inet {

class IP : public internal::IP
{
public:
  static Try<IP> parse(const std::string& value);

  explicit IP(const struct in_addr& _storage)
    : internal::IP(_storage) {};

  explicit IP(uint32_t _ip)
    : internal::IP(_ip) {};

  // Returns the struct in_addr storage.
  struct in_addr in() const
  {
    // `family_` is already set to AF_INET
    return internal::IP::in().get();
  }
};


inline Try<IP> IP::parse(const std::string& value)
{
  struct in_addr storage;

  if (inet_pton(AF_INET, value.c_str(), &storage) == 1) {
    return IP(storage);
  }

  return Error("Failed to parse IPv4: " + value);
}

} // namespace inet {


namespace inet6 {

class IP : public internal::IP
{
public:
  static Try<IP> parse(const std::string& value);

  explicit IP(const struct in6_addr& _storage)
    : internal::IP(_storage) {};

  struct in6_addr in6() const
  {
    return internal::IP::in6().get();
  }
};


inline Try<IP> IP::parse(const std::string& value)
{
  struct in6_addr storage;
  if (inet_pton(AF_INET6, value.c_str(), &storage) == 1) {
    return IP(storage);
  }

  return Error("Failed to parse IPv6: " + value);
}

} // namespace inet6 {

} // namespace net {

namespace std {
template <>
struct hash<net::inet::IP>
{
  size_t operator()(const net::inet::IP& ip)
  {
    size_t seed = 0;

    boost::hash_combine(seed, htonl(ip.in().s_addr));
    return seed;
  }
};


template <>
struct hash<net::inet6::IP>
{
  size_t operator()(const net::inet6::IP& ip)
  {
    size_t seed = 0;
    struct in6_addr in6 = ip.in6();

    boost::hash_range(seed, std::begin(in6.s6_addr), std::end(in6.s6_addr));
    return seed;
  }
};
} // namespace std {

#endif // __STOUT_IP46_HPP__
