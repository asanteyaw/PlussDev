#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;
namespace lymph = pluss::ai::lymph;

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series vec(std::vector<double> values) {
  return pluss::make_series(std::move(values), {static_cast<std::int64_t>(values.size())}, f64());
}

DataFrame frame() {
  return DataFrame({
      {"timestamp", Slate(vec({1.0, 2.0, 3.0}))},
      {"x", Slate(vec({10.0, 20.0, 30.0}))},
      {"y", Slate(vec({0.0, 1.0, 0.0}))},
  });
}

std::filesystem::path fresh_bundle_path(const std::string& stem) {
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() / (stem + "_" + std::to_string(tick));
}

ai::Step frame_gate(std::string name = "gate") {
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Gate, ai::StepCapability::Map};
  options.inputs = {"raw"};
  options.outputs = {"clean"};
  options.require_columns = {"timestamp", "x"};
  options.time_feature = "timestamp";
  options.requires_time = true;
  ai::Step::Runner runner = [](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("procedure_phase7 frame_gate expected one input.");
    }
    if (inputs.front().is<DataFrame>()) {
      return ai::Artifact("clean", ai::Artifact::Value(inputs.front().get<DataFrame>()));
    }
    if (inputs.front().is<lymph::workflow::Payload>()) {
      return ai::Artifact(
          "clean",
          ai::Artifact::Value(inputs.front().get<lymph::workflow::Payload>()));
    }
    ::pluss::throw_error("procedure_phase7 frame_gate expected DataFrame or Payload input.");
  };
  return ai::procedure::step(std::move(name), std::move(options), std::move(runner));
}

ai::Step shrink_step(bool declared) {
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"x"};
  options.outputs = {"y"};
  options.can_change_row_count = declared;
  ai::Step::Runner runner = [](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
    if (inputs.size() != 1 || !inputs.front().is<Series>()) {
      ::pluss::throw_error("procedure_phase7 shrink_step expected one Series input.");
    }
    return ai::Artifact("y", ai::Artifact::Value(vec({1.0, 2.0})));
  };
  return ai::procedure::step("shrink", std::move(options), std::move(runner));
}

ai::Step future_step() {
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Learn};
  options.inputs = {"x"};
  options.outputs = {"ok"};
  options.learns_from_future = true;
  ai::Step::Runner runner = [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact {
    return ai::Artifact("ok", ai::Artifact::Value(true));
  };
  return ai::procedure::step("future", std::move(options), std::move(runner));
}

bool contains_text(const std::vector<std::string>& values, const std::string& needle) {
  for (const std::string& value : values) {
    if (value.find(needle) != std::string::npos) return true;
  }
  return false;
}

bool run_phase7_checks(std::ostream* err = nullptr) {
  {
    const ai::Procedure proc = ai::procedure::make({frame_gate()});
    const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", frame())});
    if (!out.ok() || out.audit().size() != 1 || out.audit().front().step != "gate") {
      if (err) *err << "Procedure phase 7 should record per-step audit details for successful runs\n";
      return false;
    }
    const ai::StepTrace& audit = out.audit().front();
    if (audit.inputs.size() != 1 || audit.inputs.front().type != "DataFrame" ||
        audit.inputs.front().rows != 3 || audit.inputs.front().columns != 3 ||
        audit.output.rows != 3 || audit.row_count_changed) {
      if (err) *err << "Procedure phase 7 audit should capture DataFrame shape without false row-change reports\n";
      return false;
    }
  }

  {
    ai::ProcedureOptions options{};
    options.fail_fast = false;
    const ai::Procedure proc = ai::procedure::make({frame_gate()}, {}, options);
    const DataFrame missing_time({
        {"x", Slate(vec({10.0, 20.0, 30.0}))},
        {"y", Slate(vec({0.0, 1.0, 0.0}))},
    });
    const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", missing_time)});
    if (out.ok() || out.failed_step() != "gate" || !contains_text(out.warnings(), "timestamp")) {
      if (err) *err << "Procedure phase 7 should report schema/time contract failures without throwing when fail_fast=false\n";
      return false;
    }
  }

  {
    ai::ProcedureOptions options{};
    options.fail_fast = false;
    const ai::Procedure proc = ai::procedure::make({frame_gate()}, {}, options);
    lymph::workflow::Payload sample =
        lymph::workflow::payload::make(frame()).time("timestamp");
    const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", sample)});
    if (!out.ok() || out.audit().empty() ||
        out.audit().front().inputs.front().type != "lymph::workflow::Payload") {
      if (err) *err << "Procedure phase 7 should accept Payload time roles for time-aware steps\n";
      return false;
    }
  }

  {
    ai::ProcedureOptions options{};
    options.fail_fast = false;
    const ai::Procedure proc = ai::procedure::make({shrink_step(false)}, {}, options);
    const ai::ProcedureResult out = proc.run({ai::procedure::input("x", vec({1.0, 2.0, 3.0}))});
    if (out.ok() || out.failed_step() != "shrink" || !contains_text(out.warnings(), "row count")) {
      if (err) *err << "Procedure phase 7 should fail undeclared row-count changes\n";
      return false;
    }
  }

  {
    const ai::Procedure proc = ai::procedure::make({shrink_step(true)});
    const ai::ProcedureResult out = proc.run({ai::procedure::input("x", vec({1.0, 2.0, 3.0}))});
    if (!out.ok() || out.audit().size() != 1 || !out.audit().front().row_count_changed ||
        out.audit().front().output.rows != 2) {
      if (err) *err << "Procedure phase 7 should allow declared row-count-changing steps and mark the audit record\n";
      return false;
    }
  }

  {
    ai::ProcedureOptions options{};
    options.fail_fast = false;
    const ai::Procedure proc = ai::procedure::make({future_step()}, {}, options);
    ai::RunOptions run{};
    run.mode = ai::ProcedureMode::Learn;
    const ai::ProcedureResult blocked = proc.run({ai::procedure::input("x", vec({1.0, 2.0, 3.0}))}, run);
    if (blocked.ok() || blocked.failed_step() != "future" || !contains_text(blocked.warnings(), "future-data")) {
      if (err) *err << "Procedure phase 7 should block declared future-data access by default in Learn/Assess runs\n";
      return false;
    }
    run.allow_temporal_leakage = true;
    const ai::ProcedureResult allowed = proc.run({ai::procedure::input("x", vec({1.0, 2.0, 3.0}))}, run);
    if (!allowed.ok() || !allowed.get<bool>("ok")) {
      if (err) *err << "Procedure phase 7 should allow explicitly audited future-data runs\n";
      return false;
    }
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase7_contract");
    ai::Step gate = ai::procedure::step(
        "gate", "raw", "clean", lymph::workflow::Policy{});
    gate.options().require_columns = {"timestamp", "x"};
    gate.options().time_feature = "timestamp";
    gate.options().requires_time = true;
    const ai::Procedure proc = ai::procedure::make({gate});
    proc.save(bundle.string());
    ai::Procedure restored = ai::procedure::load(bundle.string());
    ai::ProcedureOptions options = restored.options();
    options.fail_fast = false;
    restored = ai::procedure::make(std::vector<ai::Step>(restored.steps().begin(), restored.steps().end()),
                                   std::vector<ai::Link>(restored.links().begin(), restored.links().end()),
                                   options);
    const DataFrame missing({
        {"x", Slate(vec({10.0, 20.0, 30.0}))},
    });
    const ai::ProcedureResult out = restored.run({ai::procedure::input("raw", missing)});
    std::filesystem::remove_all(bundle);
    if (out.ok() || out.failed_step() != "gate" || !contains_text(out.warnings(), "timestamp")) {
      if (err) *err << "Procedure phase 7 should persist and reload schema/time contracts\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 7 safety and observability", "[procedure][safety][audit]") {
  REQUIRE(run_phase7_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase7_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 7 checks passed\n";
  return 0;
}
#endif
