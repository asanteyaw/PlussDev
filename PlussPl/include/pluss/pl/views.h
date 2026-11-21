#pragma once

#include "pluss/pl/studio.h"

#include <map>
#include <string>
#include <vector>

// Views: the choices a plot window offers for looking at a figure differently (smoothing, a transform of the
// values, a log scale, one series in focus, small multiples, a fit line, a theme, ...). Each view is a group of
// options; a choice rewrites a copy of the figure, never the figure itself. `views::groups` lists the groups that
// make sense for a given figure (a histogram has no smoothing, a single line has no focus), and `views::apply`
// returns the figure with a set of choices applied together, so views combine.
namespace pluss::pl::views {

struct Option {
  std::string id{};
  std::string label{};
};

struct Group {
  std::string id{};
  std::string title{};
  // Where the window shows the group: "" is a menu of its own, otherwise a cascading entry under View grouped
  // with the other groups of the same section name.
  std::string section{};
  // The first option is the group's default: choosing it leaves the figure as it was drawn.
  std::vector<Option> options{};
};

// group id -> chosen option id. A group that is absent (or set to its first option) leaves the figure unchanged.
using Choices = std::map<std::string, std::string>;

// The groups that apply to `figure`, in the order the window lists them.
[[nodiscard]] std::vector<Group> groups(const Studio& figure);

// A copy of `figure` with every choice applied. A choice that names a group or an option that does not apply to
// the figure is ignored.
[[nodiscard]] Studio apply(const Studio& figure, const Choices& choices);

} // namespace pluss::pl::views
