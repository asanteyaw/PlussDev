#include "pluss/ai/nodeb/io.h"

#include "io_detail.h"
#include "pluss/ai/nodeb/registry.h"
#include "pluss/ai/nodeb/resources.h"

#include <istream>
#include <ostream>
#include <sstream>

namespace pluss::ai::nodeb::io {

namespace {

using detail::fnv1a64;
using detail::parse_hex_u64;
using detail::parse_i64;
using detail::to_hex;

constexpr std::string_view kMagicLine = "pluss_nodeb_workflow";
constexpr std::string_view kLegacyMagicV1 = "pluss_nodeb_workflow_v1";
constexpr std::string_view kLegacyMagicV2 = "pluss_nodeb_workflow_v2";

std::string language_name(Language lang) {
  return std::string(resources::language_name(lang));
}

Language parse_language(std::string_view value) {
  PLUSS_CALL_GUARD();
  const auto parsed = resources::parse_language_code(value);
  if (parsed != Language::Unknown || value == "Unknown" || value == "unknown") {
    return parsed;
  }
  ::pluss::ai::nodeb::detail::fail(
      "nodeb::io::read_workflow: unknown language '" + std::string(value) + "'.");
}

Stage resolve_stage(std::string_view name) {
  PLUSS_CALL_GUARD();
  auto& registry = registry::StageRegistry::instance();
  if (!registry.contains(name)) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: unknown stage '" + std::string(name) + "'.");
  }
  return registry.create(name);
}

void write_stage_block(std::ostream& out, const IStage& stage) {
  PLUSS_CALL_GUARD();
  const auto lines = stage.dump_state_lines();
  out << "stage_begin\n";
  out << "name=" << stage.name() << "\n";
  out << "line_count=" << lines.size() << "\n";
  for (const auto& line : lines) {
    out << line << "\n";
  }
  out << "stage_end\n";
}

Stage read_stage_block(std::istream& in) {
  PLUSS_CALL_GUARD();
  std::string line{};
  if (!std::getline(in, line) || line.rfind("name=", 0) != 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: expected stage name line.");
  }
  auto stage = resolve_stage(std::string_view(line).substr(5));
  if (!std::getline(in, line) || line.rfind("line_count=", 0) != 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: expected stage line_count.");
  }
  const auto line_count = static_cast<std::size_t>(parse_i64(std::string_view(line).substr(11)));
  std::vector<std::string> payload{};
  payload.reserve(line_count);
  for (std::size_t i = 0; i < line_count; ++i) {
    if (!std::getline(in, line)) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::io::read_workflow: unexpected EOF while reading stage payload.");
    }
    payload.push_back(line);
  }
  if (!std::getline(in, line) || line != "stage_end") {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: expected stage_end marker.");
  }
  std::vector<std::string_view> views(payload.begin(), payload.end());
  stage->load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(views));
  return stage;
}

[[nodiscard]] std::string read_remaining(std::istream& in) {
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

[[nodiscard]] Workflow read_legacy_workflow(std::istream& in, std::string_view magic) {
  PLUSS_CALL_GUARD();
  const bool legacy_v1 = magic == kLegacyMagicV1;

  Workflow workflow{};
  std::string line{};
  while (std::getline(in, line)) {
    if (line.rfind("lang=", 0) == 0) {
      workflow.lang = parse_language(std::string_view(line).substr(5));
      continue;
    }
    if (legacy_v1 && line.rfind("stage=", 0) == 0) {
      workflow.stages.push_back(resolve_stage(std::string_view(line).substr(6)));
      continue;
    }
    if (!legacy_v1 && line == "stage_begin") {
      workflow.stages.push_back(read_stage_block(in));
      continue;
    }
    if (!line.empty()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::io::read_workflow: unrecognized line '" + line + "'.");
    }
  }
  return workflow;
}

} // namespace

void write_workflow(std::ostream& out, const Workflow& workflow) {
  PLUSS_CALL_GUARD();
  std::ostringstream body{};
  for (const auto& stage : workflow.stages) {
    if (!stage) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::io::write_workflow: encountered null stage.");
    }
    write_stage_block(body, *stage);
  }
  const auto body_text = body.str();

  out << kMagicLine << "\n";
  out << "schema=" << kCurrentSchemaVersion << "\n";
  out << "lang=" << language_name(workflow.lang) << "\n";
  out << "stage_count=" << workflow.stages.size() << "\n";
  out << "checksum=" << to_hex(fnv1a64(body_text)) << "\n";
  out << body_text;
}

Workflow read_workflow(std::istream& in) {
  PLUSS_CALL_GUARD();
  std::string line{};
  if (!std::getline(in, line)) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: invalid workflow header.");
  }

  if (line == kLegacyMagicV1 || line == kLegacyMagicV2) {
    return read_legacy_workflow(in, line);
  }
  if (line != kMagicLine) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: invalid workflow header.");
  }

  std::int64_t schema = -1;
  Language lang = Language::English;
  std::int64_t stage_count = -1;
  std::uint64_t expected_checksum = 0;
  bool have_checksum = false;

  while (std::getline(in, line)) {
    if (line.rfind("schema=", 0) == 0) {
      schema = parse_i64(std::string_view(line).substr(7));
      continue;
    }
    if (line.rfind("lang=", 0) == 0) {
      lang = parse_language(std::string_view(line).substr(5));
      continue;
    }
    if (line.rfind("stage_count=", 0) == 0) {
      stage_count = parse_i64(std::string_view(line).substr(12));
      continue;
    }
    if (line.rfind("checksum=", 0) == 0) {
      expected_checksum = parse_hex_u64(std::string_view(line).substr(9));
      have_checksum = true;
      break; // checksum is always the last header line; the body follows immediately.
    }
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: unrecognized workflow header line '" + line + "'.");
  }

  if (schema < kMinLoadableSchemaVersion || schema > kCurrentSchemaVersion) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: unsupported workflow schema " + std::to_string(schema) +
        " (supports " + std::to_string(kMinLoadableSchemaVersion) + ".." +
        std::to_string(kCurrentSchemaVersion) + ").");
  }
  if (stage_count < 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: workflow header is missing stage_count.");
  }
  if (!have_checksum) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: workflow header is missing checksum.");
  }

  const auto body_text = read_remaining(in);
  const auto actual_checksum = fnv1a64(body_text);
  if (actual_checksum != expected_checksum) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: checksum mismatch, artifact is truncated or corrupted "
        "(expected " + to_hex(expected_checksum) + ", got " + to_hex(actual_checksum) + ").");
  }

  std::istringstream body_stream(body_text);
  Workflow workflow{};
  workflow.lang = lang;
  workflow.stages.reserve(static_cast<std::size_t>(stage_count));
  while (std::getline(body_stream, line)) {
    if (line == "stage_begin") {
      workflow.stages.push_back(read_stage_block(body_stream));
      continue;
    }
    if (!line.empty()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::io::read_workflow: unrecognized line '" + line + "'.");
    }
  }
  if (static_cast<std::int64_t>(workflow.stages.size()) != stage_count) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::io::read_workflow: expected " + std::to_string(stage_count) +
        " stages but found " + std::to_string(workflow.stages.size()) + ".");
  }
  return workflow;
}

} // namespace pluss::ai::nodeb::io
