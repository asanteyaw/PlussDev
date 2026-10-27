#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;

pluss::Config f64() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

bool run_procedure_phase1_checks(std::ostream* err = nullptr) {
  ai::StepOptions admit{};
  admit.capabilities = {ai::StepCapability::Admit};
  admit.outputs = {"raw"};

  ai::StepOptions map{};
  map.capabilities = {ai::StepCapability::Map};
  map.inputs = {"raw"};
  map.outputs = {"features"};

  ai::ProcedureOptions options{};
  options.workers = 2;
  options.cache = ai::ProcedureCache::ReadyArtifacts;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("gate", admit),
      ai::procedure::step("features", map),
      ai::procedure::step("store", ai::procedure::sink("risk_forecast.out")),
  }, {
      ai::procedure::link("gate", "features", "raw", "raw"),
      ai::procedure::link("features", "store", "features", "input"),
  }, options);

  if (proc.step_count() != 3 || proc.link_count() != 2 || !proc.has("features")) {
    if (err) *err << "valid Procedure should keep all steps and links\n";
    return false;
  }
  if (!proc.get("store").has(ai::StepCapability::Sink) || !proc.get("store").persistable()) {
    if (err) *err << "sink factory should create a persistable sink-capable step\n";
    return false;
  }
  if (proc.names().size() != 3 || proc.names()[0] != "gate") {
    if (err) *err << "Procedure names() should preserve step insertion order\n";
    return false;
  }

  const ai::Artifact named = ai::procedure::input("answer", std::int64_t{42});
  if (!named.named() || !named.is<std::int64_t>() || named.get<std::int64_t>() != 42) {
    if (err) *err << "procedure::input should create a named typed artifact\n";
    return false;
  }

  ai::ProcedureOptions no_validate{};
  no_validate.validate_on_make = false;
  const ai::Procedure unchecked = ai::procedure::make({
      ai::procedure::step("a"),
      ai::procedure::step("b"),
  }, {
      ai::procedure::link("a", "b"),
      ai::procedure::link("b", "a"),
  }, no_validate);
  if (!unchecked.check_message().has_value()) {
    if (err) *err << "explicit check() should catch cycles when validate_on_make is false\n";
    return false;
  }

  if (!ai::procedure::make(std::vector<ai::Step>{}, std::vector<ai::Link>{}, no_validate).check_message().has_value()) {
    if (err) *err << "empty Procedure graph should fail\n";
    return false;
  }

  if (!ai::procedure::make({
            ai::procedure::step("dup"),
            ai::procedure::step("dup"),
        }, {}, no_validate).check_message().has_value()) {
    if (err) *err << "duplicate step names should fail\n";
    return false;
  }

  if (!ai::procedure::make({
            ai::procedure::step("a"),
        }, {
            ai::procedure::link("a", "missing"),
        }, no_validate).check_message().has_value()) {
    if (err) *err << "links to missing steps should fail\n";
    return false;
  }

  if (!ai::procedure::make({
            ai::procedure::step("a"),
            ai::procedure::step("b"),
        }, {
            ai::procedure::link("a", "b"),
            ai::procedure::link("a", "b"),
        }, no_validate).check_message().has_value()) {
    if (err) *err << "duplicate links should fail\n";
    return false;
  }

  ai::StepOptions declared_output{};
  declared_output.outputs = {"ok"};
  ai::StepOptions declared_input{};
  declared_input.inputs = {"in"};
  if (!ai::procedure::make({
            ai::procedure::step("a", declared_output),
            ai::procedure::step("b", declared_input),
        }, {
            ai::procedure::link("a", "b", "missing_output", "in"),
        }, no_validate).check_message().has_value()) {
    if (err) *err << "links to undeclared output ports should fail\n";
    return false;
  }

  ai::StepOptions no_capabilities{};
  no_capabilities.capabilities = {};
  if (!ai::procedure::make({ai::procedure::step("bad", no_capabilities)}, {}, no_validate).check_message().has_value()) {
    if (err) *err << "steps with no capabilities should fail\n";
    return false;
  }

  ai::StepOptions duplicate_inputs{};
  duplicate_inputs.inputs = {"x", "x"};
  if (!ai::procedure::make({ai::procedure::step("bad", duplicate_inputs)}, {}, no_validate).check_message().has_value()) {
    if (err) *err << "duplicate input port names should fail\n";
    return false;
  }

  ai::ProcedureOptions bad_workers{};
  bad_workers.validate_on_make = false;
  bad_workers.workers = 0;
  if (!ai::procedure::make({ai::procedure::step("a")}, {}, bad_workers).check_message().has_value()) {
    if (err) *err << "non-positive worker counts should fail\n";
    return false;
  }

  return true;
}

bool run_procedure_phase2_checks(std::ostream* err = nullptr) {
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, f64());

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("twice", "x", "twice", [](const pluss::Series& input) {
        return input.mul(2.0);
      }),
      ai::procedure::step("plus", "twice", "plus", [](const pluss::Series& input) {
        return input.add(1.0);
      }),
      ai::procedure::step("store", ai::procedure::sink("phase2.out")),
  }, {
      ai::procedure::link("twice", "plus"),
      ai::procedure::link("plus", "store"),
  });

  const ai::ProcedureResult result = proc.run({ai::procedure::input("x", x)});
  const pluss::Series y = result.get<pluss::Series>("plus");
  const std::vector<double> values = y.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  if (values.size() != 3 || values[0] != 3.0 || values[1] != 5.0 || values[2] != 7.0) {
    if (err) *err << "Procedure Phase 2 should run Series callables through vectorized Series ops\n";
    return false;
  }
  if (result.trace().size() != 3 || result.trace()[0] != "ran:twice" || result.trace()[2] != "ran:store") {
    if (err) *err << "Procedure Phase 2 should record deterministic step trace order\n";
    return false;
  }
  const std::vector<double> stored =
      result.get<pluss::Series>("store").to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  if (stored != values) {
    if (err) *err << "Procedure sink should preserve the upstream artifact value under the sink step name\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 1 graph contracts") {
  REQUIRE(run_procedure_phase1_checks(&std::cerr));
}

TEST_CASE("Procedure phase 2 local executor") {
  REQUIRE(run_procedure_phase2_checks(&std::cerr));
}
#else
int main() {
  if (!run_procedure_phase1_checks(&std::cerr)) {
    return 1;
  }
  if (!run_procedure_phase2_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "procedure_phase1_test passed\n";
  return 0;
}
#endif
