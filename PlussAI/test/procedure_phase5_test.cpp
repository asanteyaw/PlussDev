#include <pluss/ai/procedure.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;

ai::Step value_step(std::string name,
                    std::string output,
                    std::int64_t value,
                    std::chrono::milliseconds delay,
                    std::atomic<int>* active = nullptr,
                    std::atomic<int>* max_active = nullptr) {
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.outputs = {output};
  ai::Step::Runner runner =
      [output = std::move(output), value, delay, active, max_active](
          const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
        if (!inputs.empty()) {
          ::pluss::throw_error("procedure_phase5 value_step expected no inputs.");
        }
        if (active != nullptr && max_active != nullptr) {
          const int now = active->fetch_add(1) + 1;
          int observed = max_active->load();
          while (now > observed && !max_active->compare_exchange_weak(observed, now)) {}
          std::this_thread::sleep_for(delay);
          active->fetch_sub(1);
        } else {
          std::this_thread::sleep_for(delay);
        }
        return ai::Artifact(output, ai::Artifact::Value(value));
      };
  return ai::procedure::step(std::move(name), std::move(options), std::move(runner));
}

ai::Step sum_step(std::string name, std::string output) {
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Join};
  options.inputs = {"lhs", "rhs"};
  options.outputs = {output};
  ai::Step::Runner runner =
      [output = std::move(output)](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
        if (inputs.size() != 2) {
          ::pluss::throw_error("procedure_phase5 sum_step expected two inputs.");
        }
        const std::int64_t sum = inputs[0].get<std::int64_t>() + inputs[1].get<std::int64_t>();
        return ai::Artifact(output, ai::Artifact::Value(sum));
      };
  return ai::procedure::step(std::move(name), std::move(options), std::move(runner));
}

bool trace_eq(const std::vector<std::string>& trace, const std::vector<std::string>& expected) {
  return trace == expected;
}

bool run_phase5_checks(std::ostream* err = nullptr) {
  {
    ai::ProcedureOptions bad{};
    bad.workers = 0;
    bad.validate_on_make = false;
    const ai::Procedure invalid = ai::procedure::make({value_step("a", "a", 1, std::chrono::milliseconds{0})},
                                                      {}, bad);
    const std::optional<std::string> issue = invalid.check_message();
    if (!issue.has_value() || issue->find("worker count") == std::string::npos) {
      if (err) *err << "Procedure should reject worker counts below one without requiring thrown diagnostics\n";
      return false;
    }
  }

  {
    std::atomic<int> active{0};
    std::atomic<int> max_active{0};
    ai::ProcedureOptions options{};
    options.workers = 2;
    const ai::Procedure proc = ai::procedure::make({
        value_step("slow", "lhs", 10, std::chrono::milliseconds{120}, &active, &max_active),
        value_step("fast", "rhs", 7, std::chrono::milliseconds{20}, &active, &max_active),
        sum_step("join", "sum"),
    }, {
        ai::procedure::link("slow", "join", "lhs", "lhs"),
        ai::procedure::link("fast", "join", "rhs", "rhs"),
    }, options);

    const ai::ProcedureResult out = proc.run();
    if (out.get<std::int64_t>("sum") != 17) {
      if (err) *err << "parallel Procedure should pass both branch artifacts into dependent join step\n";
      return false;
    }
    if (max_active.load() < 2) {
      if (err) *err << "parallel Procedure should run independent ready steps concurrently when workers > 1\n";
      return false;
    }
    if (!trace_eq(out.trace(), {"ran:slow", "ran:fast", "ran:join"})) {
      if (err) *err << "parallel Procedure should commit trace in deterministic graph order\n";
      return false;
    }
    const std::vector<ai::Artifact>& artifacts = out.artifacts();
    const bool artifact_order = artifacts.size() == 3 &&
                                artifacts[0].name() == "lhs" &&
                                artifacts[1].name() == "rhs" &&
                                artifacts[2].name() == "sum";
    if (!artifact_order) {
      if (err) *err << "parallel Procedure should commit artifacts in deterministic graph order\n";
      return false;
    }
  }

  {
    std::atomic<int> active{0};
    std::atomic<int> max_active{0};
    ai::ProcedureOptions options{};
    options.workers = 2;
    const ai::Procedure proc = ai::procedure::make({
        value_step("a", "a", 1, std::chrono::milliseconds{40}, &active, &max_active),
        value_step("b", "b", 2, std::chrono::milliseconds{40}, &active, &max_active),
        value_step("c", "c", 3, std::chrono::milliseconds{40}, &active, &max_active),
        value_step("d", "d", 4, std::chrono::milliseconds{40}, &active, &max_active),
    }, {}, options);
    const ai::ProcedureResult out = proc.run();
    if (max_active.load() != 2) {
      if (err) *err << "parallel Procedure should honor ProcedureOptions::workers as a hard local cap\n";
      return false;
    }
    if (!trace_eq(out.trace(), {"ran:a", "ran:b", "ran:c", "ran:d"})) {
      if (err) *err << "worker-capped Procedure should keep deterministic trace across chunks\n";
      return false;
    }
  }

  {
    ai::ProcedureOptions serial_options{};
    serial_options.workers = 1;
    const ai::Procedure serial = ai::procedure::make({
        value_step("a", "lhs", 3, std::chrono::milliseconds{0}),
        value_step("b", "rhs", 4, std::chrono::milliseconds{0}),
        sum_step("join", "sum"),
    }, {
        ai::procedure::link("a", "join", "lhs", "lhs"),
        ai::procedure::link("b", "join", "rhs", "rhs"),
    }, serial_options);
    const ai::ProcedureResult out = serial.run();
    if (out.get<std::int64_t>("sum") != 7 ||
        !trace_eq(out.trace(), {"ran:a", "ran:b", "ran:join"})) {
      if (err) *err << "serial Procedure path should preserve pre-Phase-5 behavior\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 5 local parallel runtime", "[procedure][parallel]") {
  REQUIRE(run_phase5_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase5_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 5 checks passed\n";
  return 0;
}
#endif
