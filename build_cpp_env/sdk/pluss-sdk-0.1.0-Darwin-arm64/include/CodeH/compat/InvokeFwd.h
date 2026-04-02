#pragma once

#include <string_view>
#include <utility>

namespace pluss::codeh {
class Timeline;
class Duration;
class TimeIndex;
struct CalendarOffset;
} // namespace pluss::codeh

namespace pluss::codeh::_compat {

struct Arg;
struct Args;

namespace detail {
struct Access;
} // namespace detail

template <class R, class Self, class... A>
R invoke(std::string_view op, const Self& self, A&&... a);

} // namespace pluss::codeh::_compat
