#include <pluss/nlp.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

// A minimal user-defined stage used to prove the registry can host stage
// types NodeB itself knows nothing about, end to end through save/load.
class MarkerStage final : public IStage {
public:
  [[nodiscard]] std::string_view name() const noexcept override { return "custom_marker"; }
  void apply(Doc& doc) const override { doc.doc_label = "marked"; }
};

template <typename Fn>
[[nodiscard]] bool throws(Fn&& fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

bool run_nodeb_registry_checks(std::ostream* err = nullptr) {
  auto& reg = registry::StageRegistry::instance();

  const std::vector<std::string> builtins = {
      "lemmatizer", "matcher", "ner", "rule_tokenizer",
      "sentencizer", "syntax", "tagger", "textcat",
  };
  for (const auto& name : builtins) {
    if (!reg.contains(name)) {
      if (err) *err << "nodeb registry should contain built-in stage '" << name << "'\n";
      return false;
    }
  }
  if (reg.contains("does_not_exist_stage")) {
    if (err) *err << "nodeb registry should not contain an unregistered stage name\n";
    return false;
  }

  const auto tagger_stage = reg.create("tagger");
  if (!tagger_stage || tagger_stage->name() != "tagger") {
    if (err) *err << "nodeb registry should construct built-in stages by name\n";
    return false;
  }

  if (!throws([&] { (void)reg.create("does_not_exist_stage"); })) {
    if (err) *err << "nodeb registry create() should throw for an unknown stage name\n";
    return false;
  }
  if (!throws([&] { (void)reg.add("", [] { return Stage{}; }); })) {
    if (err) *err << "nodeb registry add() should reject an empty stage name\n";
    return false;
  }

  const auto names = reg.names();
  if (!std::ranges::is_sorted(names)) {
    if (err) *err << "nodeb registry names() should be sorted\n";
    return false;
  }
  for (const auto& name : builtins) {
    if (std::ranges::find(names, name) == names.end()) {
      if (err) *err << "nodeb registry names() should list built-in stage '" << name << "'\n";
      return false;
    }
  }

  // Colliding with a built-in must be rejected, and must not disturb it.
  if (reg.add("tagger", [] { return Stage{}; })) {
    if (err) *err << "nodeb registry add() should refuse to overwrite an existing stage name\n";
    return false;
  }
  if (reg.create("tagger")->name() != "tagger") {
    if (err) *err << "nodeb registry rejecting a duplicate add() should not disturb the original factory\n";
    return false;
  }

  if (!reg.add("custom_marker", [] { return Stage(std::make_shared<MarkerStage>()); })) {
    if (err) *err << "nodeb registry add() should accept a new custom stage name\n";
    return false;
  }
  if (reg.add("custom_marker", [] { return Stage(std::make_shared<MarkerStage>()); })) {
    if (err) *err << "nodeb registry add() should refuse to register the same custom name twice\n";
    return false;
  }

  Workflow nlp{
      .lang = Language::English,
      .stages = {stage::tokenize(), std::make_shared<MarkerStage>()},
  };
  const auto doc = nlp.infer("Laurel writes APIs.");
  if (doc.doc_label != "marked") {
    if (err) *err << "nodeb custom stage should run as part of a workflow\n";
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_registry_custom_stage.bin";
  nlp.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  std::filesystem::remove(tmp);

  if (restored.stages.size() != 2 || restored.stages[1]->name() != "custom_marker") {
    if (err) *err << "nodeb workflow save/load should resolve custom stages through the registry\n";
    return false;
  }
  const auto restored_doc = restored.infer("Laurel writes APIs.");
  if (restored_doc.doc_label != "marked") {
    if (err) *err << "nodeb workflow save/load should preserve custom stage behavior\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb registry tranche", "[nodeb][registry]") {
  if (!run_nodeb_registry_checks(&std::cerr)) {
    FAIL("nodeb registry checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_registry_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb registry checks passed\n";
  return 0;
}
#endif
