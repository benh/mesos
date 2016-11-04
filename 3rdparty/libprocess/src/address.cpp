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

#include <process/address.hpp>

namespace process {
namespace network {
namespace inet {

const Address Address::LOOPBACK_ANY = Address(net::IP(INADDR_LOOPBACK), 0);

const Address Address::ANY_ANY = Address(net::IP(INADDR_ANY), 0);

} // namespace inet {
} // namespace network {
} // namespace process {
