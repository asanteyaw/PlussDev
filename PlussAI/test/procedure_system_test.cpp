#include <pluss/ai/procedure.h>
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

pluss::Config f64() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

pluss::Series vec(std::vector<double> values) {
  return pluss::make_series(std::move(values), {static_cast<std::int64_t>(values.size())}, f64());
}

std::filesystem::path fresh_bundle_path(const std::string& stem) {
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() / (stem + "_" + std::to_string(tick));
}

bool text_has(const std::string& value, const std::string& needle) {
  return value.find(needle) != std::string::npos;
}

bool warnings_have(const std::vector<std::string>& values, const std::string& needle) {
  for (const std::string& value : values) {
    if (text_has(value, needle)) return true;
  }
  return false;
}

ai::Procedure branch_graph(std::int64_t workers) {
  ai::StepOptions join{};
  join.capabilities = {ai::StepCapability::Join};
  join.inputs = {"centered", "energy"};
  join.outputs = {"score"};

  ai::ProcedureOptions options{};
  options.workers = workers;

  return ai::procedure::make({
      ai::procedure::step("center", "x", "centered", [](const pluss::Series& x) {
        return x.sub(x.mean());
      }),
      ai::procedure::step("energy", "x", "energy", [](const pluss::Series& x) {
        return x.mul(x);
      }),
      ai::procedure::step("score", join, [](const std::vector<ai::Artifact>& inputs,
                                            const ai::RunOptions&) -> ai::Artifact {
        if (inputs.size() != 2) {
          ::pluss::throw_error("procedure_system_test score: expected two inputs.");
        }
        return ai::Artifact("score",
                            ai::Artifact::Value(inputs[0].get<pluss::Series>().add(inputs[1].get<pluss::Series>())));
      }),
      ai::procedure::step("store", ai::procedure::sink("score.out")),
  }, {
      ai::procedure::link("center", "score", "centered", "centered"),
      ai::procedure::link("energy", "score", "energy", "energy"),
      ai::procedure::link("score", "store", "score", "input"),
  }, options);
}

bool bind_triple_once() {
  static bool bound = false;
  if (bound) return true;
  ai::procedure::bind("procedure.system.triple", [](const pluss::Series& x) {
    return x.mul(3.0);
  });
  bound = true;
  return true;
}

bool run_procedure_system_checks(std::ostream* err = nullptr) {
  {
    const pluss::Series x = vec({1.0, 2.0, 4.0, 8.0});
    const ai::ProcedureResult serial = branch_graph(1).run({ai::procedure::input("x", x)});
    const ai::ProcedureResult parallel = branch_graph(4).run({ai::procedure::input("x", x)});
    const std::vector<double> a = serial.get<pluss::Series>("score").to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    const std::vector<double> b = parallel.get<pluss::Series>("score").to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    if (a != b || a != std::vector<double>{-1.75, 2.25, 16.25, 68.25}) {
      if (err) *err << "Procedure system should produce identical Series results in serial and parallel execution\n";
      return false;
    }
    if (serial.audit().size() != 4 || parallel.audit().size() != 4 ||
        serial.get<pluss::Series>("store").numel() != 4) {
      if (err) *err << "Procedure system should audit all steps and let sinks expose the terminal artifact\n";
      return false;
    }
  }

  {
    ai::ProcedureOptions options{};
    options.fail_fast = false;
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step("needs_x", "x", "y", [](const pluss::Series& x) {
          return x.add(1.0);
        }),
    }, {}, options);
    const ai::ProcedureResult out = proc.run();
    if (out.ok() || out.failed_step() != "needs_x" || !warnings_have(out.warnings(), "missing input artifact")) {
      if (err) *err << "Procedure system should return a soft failed result for missing artifacts when fail_fast=false\n";
      return false;
    }
  }

  {
    bool wrong_type_blocked = false;
    try {
      (void)ai::procedure::input("flag", true).get<pluss::Series>();
    } catch (const pluss::Error& e) {
      wrong_type_blocked = text_has(e.what(), "requested type");
    }
    if (!wrong_type_blocked) {
      if (err) *err << "Procedure Artifact should reject wrong typed reads through Pluss errors\n";
      return false;
    }
  }

  {
    (void)bind_triple_once();
    bool duplicate_blocked = false;
    try {
      ai::procedure::bind("procedure.system.triple", [](const pluss::Series& x) {
        return x;
      });
    } catch (const pluss::Error& e) {
      duplicate_blocked = text_has(e.what(), "duplicate contract key");
    }
    if (!duplicate_blocked) {
      if (err) *err << "Procedure registry should reject duplicate custom persistence keys\n";
      return false;
    }

    const auto bundle = fresh_bundle_path("procedure_system_registry");
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step("triple", "x", "y", [](const pluss::Series& x) {
          return x.mul(3.0);
        }, "procedure.system.triple"),
        ai::procedure::step("store", ai::procedure::sink("triple.out")),
    }, {
        ai::procedure::link("triple", "store", "y", "input"),
    });
    proc.save(bundle.string());
    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("x", vec({2.0, 5.0}))});
    const std::vector<double> got = out.get<pluss::Series>("store").to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    std::filesystem::remove_all(bundle);
    if (got != std::vector<double>{6.0, 15.0}) {
      if (err) *err << "Procedure system should reload registry-backed callable steps and preserve sink output\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure system end-to-end contracts", "[procedure][system]") {
  REQUIRE(run_procedure_system_checks(&std::cerr));
}
#else
int main() {
  if (!run_procedure_system_checks(&std::cerr)) return 1;
  std::cout << "procedure system checks passed\n";
  return 0;
}
#endif
