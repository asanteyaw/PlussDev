#pragma once

#include "pluss/subsp/math/function.h"
#include "pluss/subsp/math/operators.h"

#include <iosfwd>
#include <string_view>

namespace pluss::subsp::math::io {

void save(const Function &function, std::ostream &output);
void save(const Function &function, std::string_view path);
[[nodiscard]] Function load_function(std::istream &input);
[[nodiscard]] Function load_function(std::string_view path);

void save(const operators::Operator &op, std::ostream &output);
void save(const operators::Operator &op, std::string_view path);
[[nodiscard]] operators::Operator load_operator(std::istream &input);
[[nodiscard]] operators::Operator load_operator(std::string_view path);

} // namespace pluss::subsp::math::io
