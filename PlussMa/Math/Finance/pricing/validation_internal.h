#pragma once

#include "pluss/subsp/math/finance/module.h"

#include <string>
#include <vector>

// INTERNAL: not part of the public API (not under include/, not in an umbrella).  Users reach it only through Field::validate() and
// Model::validate(), which throw.
//
// One place that decides whether a Field (or any of its parts) is fit to price.  A check never stops at the first problem: it collects
// every one into a Report, so a user fixing a configuration sees all of it at once.  Errors make require() throw; warnings do not.
namespace pluss::subsp::math::finance::internal::validation {

enum class Severity : std::uint8_t { Error, Warning };

struct Issue {
  Severity severity = Severity::Error;
  std::string where;    // e.g. "model.Heston.rho", "controls.engine", "instrument.exercise_times"
  std::string message;  // what is wrong and what would fix it
};

struct Report {
  std::vector<Issue> issues;

  [[nodiscard]] CODEM_API bool ok() const;  // no errors (warnings allowed)
  [[nodiscard]] CODEM_API std::vector<Issue> errors() const;
  [[nodiscard]] CODEM_API std::vector<Issue> warnings() const;
  [[nodiscard]] CODEM_API std::string text() const;  // one line per issue
  CODEM_API void raise(const std::string& context) const;  // throws with every error when there is one
  CODEM_API void merge(Report other);
};

// The engines that can price a model family.
CODEM_API std::vector<module::Engine> supported_engines(model::Kind kind);
CODEM_API std::string engine_label(module::Engine engine);

CODEM_API Report check(const module::Model& model);
CODEM_API Report check(const instrument::Vanilla& instrument);
CODEM_API Report check(const market::State& market);
// Controls against the model and instrument they will price: engine support, style support and the settings the chosen engine reads.
CODEM_API Report check(const module::Controls& controls, const module::Model& model, const instrument::Vanilla& instrument);
// Everything above, plus the agreement of sizes and the lattice/regression conditions that need several members.
CODEM_API Report check(const module::Field& field);

CODEM_API void require(const module::Model& model);
CODEM_API void require(const module::Field& field);

}  // namespace pluss::subsp::math::finance::internal::validation
