#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series vec(std::vector<double> values) {
  return pluss::make_series(std::move(values), {static_cast<std::int64_t>(values.size())}, f64());
}

std::filesystem::path fresh_bundle_path(const std::string& stem) {
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() / (stem + "_" + std::to_string(tick));
}

std::string slurp(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) {
    ::pluss::throw_error("procedure_phase8 slurp: missing file \"" + path.string() + "\".");
  }
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

bool replace_once(std::filesystem::path path, const std::string& needle, const std::string& repl) {
  std::string text = slurp(path);
  const std::size_t pos = text.find(needle);
  if (pos == std::string::npos) return false;
  text.replace(pos, needle.size(), repl);
  std::ofstream out(path, std::ios::trunc);
  out << text;
  return true;
}

ai::Step offset_step(double offset) {
  static bool bound = false;
  if (!bound) {
    ai::procedure::bind(ai::procedure::CustomContract{
        .key = "custom::offset",
        .persist_schema_version = 2,
        .component_schema_version = 5,
        .load = [](std::string,
                   ai::StepOptions,
                   std::string_view path,
                   const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
          std::ifstream in{std::string(path)};
          if (!in) {
            ::pluss::throw_error("procedure_phase8 offset loader: missing artifact.");
          }
          double loaded = 0.0;
          in >> loaded;
          return offset_step(loaded);
        },
        .compatible = [](std::int64_t persist_schema,
                         std::int64_t component_schema,
                         const std::vector<std::pair<std::string, std::string>>&) -> std::optional<std::string> {
          if (persist_schema != 2) return "persist schema requires migration to 2";
          if (component_schema != 5) return "component schema requires rebuild to 5";
          return std::nullopt;
        },
    });
    bound = true;
  }

  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"x"};
  options.outputs = {"y"};
  options.component = "custom::offset";

  auto shared = std::make_shared<double>(offset);
  ai::Step::Runner runner = [shared](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
    if (inputs.size() != 1 || !inputs.front().is<Series>()) {
      ::pluss::throw_error("procedure_phase8 offset step: expected one Series input.");
    }
    return ai::Artifact("y", ai::Artifact::Value(inputs.front().get<Series>().add(*shared)));
  };

  return ai::procedure::step(
      "offset",
      std::move(options),
      ai::procedure::CustomPersist{
          .key = "custom::offset",
          .fields = {{"mode", "shift"}},
          .save = [shared](std::string_view path) {
            std::ofstream out(std::string(path), std::ios::trunc);
            if (!out) {
              ::pluss::throw_error("procedure_phase8 offset saver: could not open artifact.");
            }
            out << *shared << "\n";
          },
      },
      std::move(runner));
}

ai::Step doubled_callable_step() {
  static bool bound = false;
  if (!bound) {
    ai::procedure::bind("callable::double", [](const Series& input) {
      return input.mul(2.0);
    });
    bound = true;
  }
  return ai::procedure::step(
      "double",
      "x",
      "y",
      [](const Series& input) {
        return input.mul(2.0);
      },
      "callable::double");
}

bool run_phase8_checks(std::ostream* err = nullptr) {
  {
    const auto bundle = fresh_bundle_path("procedure_phase8_manifest");
    const ai::Procedure proc = ai::procedure::make({offset_step(1.5)});
    proc.save(bundle.string());

    const std::string manifest = slurp(bundle / "manifest.plussproc");
    const std::string summary = slurp(bundle / "audit" / "summary.txt");
    const std::string contracts = slurp(bundle / "audit" / "contracts.txt");
    const std::string artifacts = slurp(bundle / "audit" / "artifacts.txt");

    if (!contains(manifest, "producer=pluss::ai::Procedure") ||
        !contains(manifest, "producer_version=unversioned") ||
        !contains(manifest, "external_code_policy=reject") ||
        !contains(manifest, "custom_steps=1")) {
      if (err) *err << "Procedure phase 8 should write richer manifest metadata for producer and persistence policy\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    if (!contains(summary, "reloadable_steps=1") ||
        !contains(summary, "custom_steps=1") ||
        !contains(summary, "external_code_policy=reject")) {
      if (err) *err << "Procedure phase 8 should emit an audit summary with runtime and persistence counts\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    if (!contains(contracts, "offset\tcustom::offset\tcustom::offset\t2\t5\t1\t1") ||
        !contains(artifacts, "offset\tsteps/0_offset\t")) {
      if (err) *err << "Procedure phase 8 should emit contract and artifact audit tables for persisted steps\n";
      std::filesystem::remove_all(bundle);
      return false;
    }

    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("x", vec({1.0, 2.0}))});
    const std::vector<double> got = out.get<Series>("y").to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (got != std::vector<double>{2.5, 3.5}) {
      if (err) *err << "Procedure phase 8 should restore contract-backed custom steps through the bound registry\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    std::filesystem::remove_all(bundle);
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase8_schema_mismatch");
    ai::procedure::make({offset_step(2.0)}).save(bundle.string());
    if (!replace_once(bundle / "manifest.plussproc", "schema=1\n", "schema=99\n")) {
      if (err) *err << "Procedure phase 8 schema mismatch test could not rewrite manifest schema\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    bool blocked = false;
    try {
      (void)ai::procedure::load(bundle.string());
    } catch (const std::exception& e) {
      blocked = contains(e.what(), "unsupported manifest schema version");
    }
    std::filesystem::remove_all(bundle);
    if (!blocked) {
      if (err) *err << "Procedure phase 8 should reject unsupported manifest schema versions during reload\n";
      return false;
    }
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase8_callable_registry");
    const ai::Procedure proc = ai::procedure::make({doubled_callable_step()});
    proc.save(bundle.string());
    const std::string manifest = slurp(bundle / "manifest.plussproc");
    const std::string summary = slurp(bundle / "audit" / "summary.txt");
    if (!contains(manifest, "external_code_policy=registry") ||
        !contains(summary, "external_ref_steps=1")) {
      if (err) *err << "Procedure phase 8 should mark registry-backed callable persistence explicitly in the manifest and audit summary\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("x", vec({1.5, 3.0}))});
    const std::vector<double> got = out.get<Series>("y").to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::filesystem::remove_all(bundle);
    if (got != std::vector<double>{3.0, 6.0}) {
      if (err) *err << "Procedure phase 8 should reload registry-backed callable steps without component artifacts\n";
      return false;
    }
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase8_custom_migration");
    ai::procedure::make({offset_step(2.0)}).save(bundle.string());
    if (!replace_once(bundle / "manifest.plussproc", "persist_schema=2\n", "persist_schema=1\n")) {
      if (err) *err << "Procedure phase 8 migration test could not rewrite custom persist schema\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    bool blocked = false;
    try {
      (void)ai::procedure::load(bundle.string());
    } catch (const std::exception& e) {
      blocked = contains(e.what(), "requires migration to 2");
    }
    std::filesystem::remove_all(bundle);
    if (!blocked) {
      if (err) *err << "Procedure phase 8 should route custom contract migration gates through bind(...) compatibility hooks\n";
      return false;
    }
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase8_custom_component");
    ai::procedure::make({offset_step(2.0)}).save(bundle.string());
    if (!replace_once(bundle / "manifest.plussproc", "component_schema=5\n", "component_schema=4\n")) {
      if (err) *err << "Procedure phase 8 component mismatch test could not rewrite custom component schema\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    bool blocked = false;
    try {
      (void)ai::procedure::load(bundle.string());
    } catch (const std::exception& e) {
      blocked = contains(e.what(), "requires rebuild to 5");
    }
    std::filesystem::remove_all(bundle);
    if (!blocked) {
      if (err) *err << "Procedure phase 8 should reject custom component schema drift through the compatibility gate\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 8 contract registry and manifest hardening", "[procedure][persist][hardening]") {
  REQUIRE(run_phase8_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase8_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 8 checks passed\n";
  return 0;
}
#endif
