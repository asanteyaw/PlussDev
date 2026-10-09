#pragma once

#include "MSharp/RangeView.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace pluss::codeh {
class Timeline;
class Duration;
class TimelineIndexAccessor;
struct CalendarOffset;
} // namespace pluss::codeh
namespace pluss::codem {
class Series;
} // namespace pluss::codem

namespace pluss::codeh::_compat {

struct Arg {
  using Storage = std::variant<
      std::monostate,
      const ::pluss::codeh::Timeline*,
      const ::pluss::codeh::Duration*,
      const ::pluss::codeh::TimelineIndexAccessor*,
      const ::pluss::codeh::CalendarOffset*,
      const ::pluss::codem::Series*,
      std::int64_t,
      bool,
      std::string
  >;

  Storage v;
  Arg() : v(std::monostate{}) {}
  Arg(const ::pluss::codeh::Timeline& x) : v(&x) {}
  Arg(const ::pluss::codeh::Duration& x) : v(&x) {}
  Arg(const ::pluss::codeh::TimelineIndexAccessor& x) : v(&x) {}
  Arg(const ::pluss::codeh::CalendarOffset& x) : v(&x) {}
  Arg(const ::pluss::codem::Series& x) : v(&x) {}
  Arg(std::int64_t x) : v(x) {}
  Arg(int x) : v(static_cast<std::int64_t>(x)) {}
  Arg(bool x) : v(x) {}
  Arg(std::string x) : v(std::move(x)) {}
  Arg(std::string_view x) : v(std::string(x)) {}
  Arg(const char* x) : v(std::string(x ? x : "")) {}

  template <class E, std::enable_if_t<std::is_enum_v<std::decay_t<E>>, int> = 0>
  Arg(E e) : v(static_cast<std::int64_t>(e)) {}
};

struct Args;
using ArgView = ::pluss::msharp::RangeView::Of<Arg>;

namespace detail {
struct Access;
} // namespace detail

template <class R, class Self>
R invoke(std::string_view op, const Self& self, ArgView args = {});

template <class R, class Self>
R invoke(std::string_view op, const Self& self, std::initializer_list<Arg> args);

} // namespace pluss::codeh::_compat
