#include <pluss/nlp.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

void write_text_file(const std::filesystem::path& path, std::string_view content) {
  std::ofstream out{path};
  out << content;
}

[[nodiscard]] std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in{path};
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

// Rewrites the value of a single `key=` header line in a workflow artifact,
// leaving every other byte (including the checksum and stage payload)
// untouched, so the rewritten field can be tested in isolation.
[[nodiscard]] std::string with_header_value(std::string_view content,
                                            std::string_view key,
                                            std::string_view new_value) {
  std::istringstream in{std::string(content)};
  std::ostringstream out{};
  std::string line{};
  bool replaced = false;
  while (std::getline(in, line)) {
    if (!replaced && line.rfind(key, 0) == 0) {
      out << key << new_value << "\n";
      replaced = true;
      continue;
    }
    out << line << "\n";
  }
  return out.str();
}

template <typename Fn>
[[nodiscard]] bool throws(Fn&& fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

bool run_nodeb_io_checks(std::ostream* err = nullptr) {
  const auto dir = std::filesystem::temp_directory_path();

  // --- v3 round trip through the public Workflow::save/load API ---
  Workflow nlp{
      .lang = Language::English,
      .stages = {stage::tokenize(), stage::lemmatize(), stage::textcat()},
  };
  const std::vector<std::string> train_docs = {
      "Compiler errors break builds", "C++ headers need cleanup",
      "Fresh bread and soup", "Bake a sweet cake",
  };
  const std::vector<std::string> train_labels = {"tech", "tech", "food", "food"};
  nlp.learn(train_docs, train_labels);

  const auto v3_path = dir / "nodeb_io_v3_roundtrip.bin";
  nlp.save(v3_path.string());
  const auto v3_bytes = read_text_file(v3_path);
  if (v3_bytes.rfind("pluss_nodeb_workflow\n", 0) != 0) {
    if (err) *err << "nodeb io v3 artifacts should start with the current unversioned magic line\n";
    return false;
  }
  if (v3_bytes.find("\nschema=3\n") == std::string::npos) {
    if (err) *err << "nodeb io v3 artifacts should declare schema=3\n";
    return false;
  }
  if (v3_bytes.find("\nchecksum=") == std::string::npos) {
    if (err) *err << "nodeb io v3 artifacts should carry a checksum line\n";
    return false;
  }

  const auto restored = Workflow::load(v3_path.string());
  const auto before = nlp.infer(std::vector<std::string>{"Headers and compiler warnings"});
  const auto after = restored.infer(std::vector<std::string>{"Headers and compiler warnings"});
  if (before[0].doc_label != after[0].doc_label || before[0].doc_label.empty()) {
    if (err) *err << "nodeb io v3 round trip should preserve trained textcat behavior\n";
    return false;
  }
  std::filesystem::remove(v3_path);

  // --- legacy v1 artifacts (name-only, no persisted per-stage state) ---
  const auto v1_path = dir / "nodeb_io_legacy_v1.bin";
  write_text_file(v1_path,
                  "pluss_nodeb_workflow_v1\n"
                  "lang=English\n"
                  "stage=rule_tokenizer\n");
  const auto v1_workflow = Workflow::load(v1_path.string());
  if (v1_workflow.stages.size() != 1 || v1_workflow.stages[0]->name() != "rule_tokenizer") {
    if (err) *err << "nodeb io should load legacy v1 artifacts\n";
    return false;
  }
  const auto v1_doc = v1_workflow.infer("Hi there");
  if (v1_doc.tokens.size() != 2) {
    if (err) *err << "nodeb io legacy v1 stages should run with default options after load\n";
    return false;
  }
  // Re-saving a legacy artifact must upgrade it to the current schema.
  const auto v1_resaved_path = dir / "nodeb_io_legacy_v1_resaved.bin";
  v1_workflow.save(v1_resaved_path.string());
  const auto resaved_bytes = read_text_file(v1_resaved_path);
  if (resaved_bytes.rfind("pluss_nodeb_workflow\n", 0) != 0) {
    if (err) *err << "nodeb io re-saving a legacy artifact should upgrade it to the current schema\n";
    return false;
  }
  std::filesystem::remove(v1_path);
  std::filesystem::remove(v1_resaved_path);

  // --- legacy v2 artifacts (full per-stage state, no schema/checksum envelope) ---
  const auto v2_path = dir / "nodeb_io_legacy_v2.bin";
  write_text_file(v2_path,
                  "pluss_nodeb_workflow_v2\n"
                  "lang=English\n"
                  "stage_begin\n"
                  "name=rule_tokenizer\n"
                  "line_count=2\n"
                  "keep_punct=1\n"
                  "use_language_exceptions=1\n"
                  "stage_end\n");
  const auto v2_workflow = Workflow::load(v2_path.string());
  if (v2_workflow.stages.size() != 1 || v2_workflow.stages[0]->name() != "rule_tokenizer") {
    if (err) *err << "nodeb io should load legacy v2 artifacts\n";
    return false;
  }
  std::filesystem::remove(v2_path);

  // --- v3 corruption/consistency guards ---
  Workflow tiny{.lang = Language::English, .stages = {stage::tokenize()}};
  const auto tiny_path = dir / "nodeb_io_v3_guard.bin";
  tiny.save(tiny_path.string());
  const auto tiny_bytes = read_text_file(tiny_path);

  if (!throws([&] {
        const auto bad_path = dir / "nodeb_io_v3_bad_schema.bin";
        write_text_file(bad_path, with_header_value(tiny_bytes, "schema=", "99"));
        auto loaded = Workflow::load(bad_path.string());
        std::filesystem::remove(bad_path);
        static_cast<void>(loaded);
      })) {
    if (err) *err << "nodeb io read_workflow should reject an unsupported schema version\n";
    return false;
  }

  if (!throws([&] {
        const auto bad_path = dir / "nodeb_io_v3_bad_count.bin";
        write_text_file(bad_path, with_header_value(tiny_bytes, "stage_count=", "2"));
        auto loaded = Workflow::load(bad_path.string());
        std::filesystem::remove(bad_path);
        static_cast<void>(loaded);
      })) {
    if (err) *err << "nodeb io read_workflow should reject a stage_count that does not match the payload\n";
    return false;
  }

  if (!throws([&] {
        const auto bad_path = dir / "nodeb_io_v3_truncated.bin";
        // Drop the trailing bytes so the checksum no longer matches the body.
        write_text_file(bad_path, tiny_bytes.substr(0, tiny_bytes.size() > 8 ? tiny_bytes.size() - 8 : 0));
        auto loaded = Workflow::load(bad_path.string());
        std::filesystem::remove(bad_path);
        static_cast<void>(loaded);
      })) {
    if (err) *err << "nodeb io read_workflow should detect a truncated/corrupted artifact via checksum\n";
    return false;
  }

  std::filesystem::remove(tiny_path);
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb io tranche", "[nodeb][io]") {
  if (!run_nodeb_io_checks(&std::cerr)) {
    FAIL("nodeb io checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_io_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb io checks passed\n";
  return 0;
}
#endif
