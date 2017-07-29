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

#ifndef __STOUT_LAMBDA_HPP__
#define __STOUT_LAMBDA_HPP__

#include <functional>

namespace lambda {

using std::bind;
using std::cref;
using std::function;
using std::ref;

using namespace std::placeholders;


template <
  template <typename...> class Iterable,
  typename F,
  typename U,
  typename V = typename std::result_of<F(U)>::type>
Iterable<V> map(F&& f, const Iterable<U>& input)
{
  Iterable<V> output;
  std::transform(
      input.begin(),
      input.end(),
      std::inserter(output, output.begin()),
      std::forward<F>(f));
  return output;
}


template <
  template <typename...> class OutputIterable,
  template <typename...> class InputIterable,
  typename F,
  typename U,
  typename V = typename std::result_of<F(U)>::type>
OutputIterable<V> map(F&& f, const InputIterable<U>& input)
{
  OutputIterable<V> output;
  std::transform(
      input.begin(),
      input.end(),
      std::inserter(output, output.begin()),
      std::forward<F>(f));
  return output;
}

} // namespace lambda {

#endif // __STOUT_LAMBDA_HPP__
