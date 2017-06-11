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

#ifndef __STOUT_VARIANT_HPP__
#define __STOUT_VARIANT_HPP__

#include <boost/variant.hpp>

#include <stout/overload.hpp>
#include <stout/traits.hpp>
#include <stout/try.hpp>

template <typename... Ts>
class Variant : public boost::variant<Ts...>
{
public:
  // We provide a basic constuctor that can take any universal type
  // from `Ts`. Note that we needed to use SFINAE in order to keep the
  // compiler from trying to use this constructor instead of the
  // default copy constructor, move constructor, etc.
  template <typename T,
            typename = typename std::enable_if<
              AtLeastOneIsSameOrConvertible<T, Ts...>::value>::type>
  Variant(T&& t) : boost::variant<Ts...>(std::forward<T>(t)) {}

  template <typename... Fs>
  auto visit(Fs&&... fs) const
    -> decltype(boost::apply_visitor(overload(std::forward<Fs>(fs)...), *this))
  {
    return boost::apply_visitor(overload(std::forward<Fs>(fs)...), *this);
  }

  template <typename... Fs>
  auto visit(Fs&&... fs)
    -> decltype(boost::apply_visitor(overload(std::forward<Fs>(fs)...), *this))
  {
    return boost::apply_visitor(overload(std::forward<Fs>(fs)...), *this);
  }

  class EqualityVisitor : public boost::static_visitor<bool>
  {
  public:
    EqualityVisitor(const Variant& that) : that(that) {}

    template <typename T>
    bool operator()(const T& lhs) const
    {
      const T* rhs = boost::get<T>(&that);
      if (rhs != nullptr) {
        return lhs == *rhs;
      }
      return false;
    }

    const Variant& that;
  };

  // NOTE: we've found that the default implementation of `operator==`
  // with `boost::variant` requires every type be comparable to every
  // other type, which is not always the case or useful. This
  // implementation does not require that as we only compare things of
  // the same type.
  bool operator==(const Variant& that) const
  {
    return boost::apply_visitor(EqualityVisitor(that), *this);

    // TODO(benh): With C++14 generic lambdas:
    // return visit(
    //     [&that](const auto& lhs) {
    //       using T = typename std::decay<decltype(lhs)>::type;
    //       const T* rhs = boost::get<T>(&that);
    //       if (rhs != nullptr) {
    //         return lhs == *rhs;
    //       }
    //       return false;
    //     });
  }

  bool operator!=(const Variant& that) const
  {
    return !(*this == that);
  }
};

#endif // __STOUT_VARIANT_HPP__
