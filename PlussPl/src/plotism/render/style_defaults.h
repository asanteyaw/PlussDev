#pragma once

#include "pluss/pl/style.h"

namespace pluss::pl::style {

// The style the figure being drawn uses: the one the Studio was given, or the defaults of StyleOptions. It is set for
// the duration of a draw by a Scope and read through look().
[[nodiscard]] const StyleOptions& active() noexcept;

class Scope {
 public:
  explicit Scope(const StyleOptions& options) noexcept;
  ~Scope();
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;

 private:
  const StyleOptions* previous_;
};

}  // namespace pluss::pl::style

[[nodiscard]] inline const pluss::pl::StyleOptions& look() noexcept { return pluss::pl::style::active(); }
