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

#include <string>

#include <gmock/gmock.h>

#include <process/gtest.hpp>
#include <process/future.hpp>
#include <process/socket.hpp>

#include <stout/gtest.hpp>
#include <stout/try.hpp>

namespace unix = process::network::unix;

using process::Future;

using std::string;


TEST(SocketTest, Unix)
{
  Try<unix::Socket> server = unix::Socket::create();
  ASSERT_SOME(server);

  Try<unix::Socket> client = unix::Socket::create();
  ASSERT_SOME(client);

  unix::Address address("\0socket");

  ASSERT_SOME(server->bind(address));
  ASSERT_SOME(server->listen(1));

  Future<unix::Socket> socket = server->accept();

  AWAIT_READY(client->connect(address));
  AWAIT_READY(socket);

  const string data = "Hello World!";

  AWAIT_READY(client->send(data));
  AWAIT_EQ(data, socket->recv(data.size()));

  AWAIT_READY(socket->send(data));
  AWAIT_EQ(data, client->recv(data.size()));
}
