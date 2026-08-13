#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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
using pluss::table::Thread;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series vec(std::vector<double> values) {
  return pluss::make_series(std::move(values), {static_cast<std::int64_t>(values.size())}, f64());
}

DataFrame raw_frame() {
  return DataFrame({
      {"id", Thread(vec({100, 101, 102}))},
      {"x", Thread(vec({1.0, 2.0, 3.0}))},
      {"y", Thread(vec({4.0, 5.0, 6.0}))},
  });
}

std::filesystem::path fresh_bundle_path(const std::string& stem) {
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() / (stem + "_" + std::to_string(tick));
}

ai::Step scale_step(double factor) {
  static bool bound = false;
  if (!bound) {
    ai::procedure::bind(
        "custom::scale",
        [](std::string name,
           ai::StepOptions options,
           std::string_view path,
           const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
          std::ifstream in{std::string(path)};
          if (!in) {
            ::pluss::throw_error("procedure_phase6 scale_step loader: missing factor artifact.");
          }
          double loaded = 1.0;
          in >> loaded;
          return scale_step(loaded);
        });
    bound = true;
  }

  auto shared_factor = std::make_shared<double>(factor);
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"x"};
  options.outputs = {"y"};
  options.persistable = true;
  options.component = "custom::scale";

  ai::StepPersistSpec persist{};
  persist.key = "custom::scale";
  persist.schema_version = 1;
  persist.component_schema_version = 1;
  persist.reloadable = true;
  persist.custom = true;
  persist.save = [shared_factor](std::string_view path) {
    std::ofstream out(std::string(path), std::ios::trunc);
    if (!out) {
      ::pluss::throw_error("procedure_phase6 scale_step saver: could not open artifact.");
    }
    out << *shared_factor << "\n";
  };

  ai::Step::Runner runner =
      [shared_factor](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
        if (inputs.size() != 1 || !inputs.front().is<Series>()) {
          ::pluss::throw_error("procedure_phase6 scale_step expected one Series input.");
        }
        return ai::Artifact("y", ai::Artifact::Value(inputs.front().get<Series>().mul(*shared_factor)));
      };

  return ai::procedure::step("scale", std::move(options), std::move(persist), std::move(runner));
}

std::vector<double> as_vec(const Series& values) {
  return values.to(pluss::pCPU, ScalarType::Float64).reshape({values.numel()}).to_vec<double>();
}

bool run_phase6_checks(std::ostream* err = nullptr) {
  {
    const auto bundle = fresh_bundle_path("procedure_phase6_policy");
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step("gate", "raw", "clean", lymph::Policy{.drop_columns = {"id"}}),
        ai::procedure::step("out", ai::procedure::sink("phase6.clean")),
    }, {
        ai::procedure::link("gate", "out"),
    });
    proc.save(bundle.string());

    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("raw", raw_frame())});
    const DataFrame clean = out.get<DataFrame>("clean");
    if (clean.get("id").has_value() || !clean.get("x").has_value() || !clean.get("y").has_value()) {
      if (err) *err << "Procedure phase 6 should reload a persisted Policy step and preserve its admission behavior\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    const DataFrame sink_out = out.get<DataFrame>("out");
    if (sink_out.get("id").has_value()) {
      if (err) *err << "Procedure phase 6 sink reload should preserve downstream flow under the sink step name\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    std::filesystem::remove_all(bundle);
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase6_custom");
    const ai::Procedure proc = ai::procedure::make({scale_step(3.0)});
    proc.save(bundle.string());

    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("x", vec({2.0, 4.0, 6.0}))});
    if (as_vec(out.get<Series>("y")) != std::vector<double>{6.0, 12.0, 18.0}) {
      if (err) *err << "Procedure phase 6 should restore custom persistable steps through the local loader registry\n";
      std::filesystem::remove_all(bundle);
      return false;
    }

    const auto artifact = bundle / "steps" / "0_scale";
    {
      std::ofstream out_corrupt(artifact, std::ios::trunc);
      out_corrupt << "999\n";
    }
    bool checksum_failed = false;
    try {
      (void)ai::procedure::load(bundle.string());
    } catch (const std::exception& e) {
      checksum_failed = std::string(e.what()).find("checksum mismatch") != std::string::npos;
    }
    if (!checksum_failed) {
      if (err) *err << "Procedure phase 6 should reject corrupted persisted component artifacts via checksum validation\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    std::filesystem::remove_all(bundle);
  }

  {
    const ai::Procedure nonpersist = ai::procedure::make({
        ai::procedure::step("twice", "x", "y", [](const Series& input) {
          return input.mul(2.0);
        }),
    });
    bool threw = false;
    try {
      nonpersist.save(fresh_bundle_path("procedure_phase6_nonpersist").string());
    } catch (const std::exception& e) {
      threw = std::string(e.what()).find("not persistable") != std::string::npos;
    }
    if (!threw) {
      if (err) *err << "Procedure phase 6 should fail loudly when a callable-only step has no persistence contract\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 6 persistence and registry hooks", "[procedure][persist]") {
  REQUIRE(run_phase6_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase6_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 6 checks passed\n";
  return 0;
}
#endif
