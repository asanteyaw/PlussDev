#pragma once

#include "pluss/ai/nodeb/workflow.h"

#include <cstdint>
#include <iosfwd>

namespace pluss::ai::nodeb::io {

// Workflow artifact schema history:
//   v1 - stage names only; no persisted per-stage state.
//   v2 - full per-stage state blocks (name + line-count-prefixed payload).
//   v3 - adds an explicit `schema=` line, a `stage_count=` line for
//        structural validation, and an FNV-1a integrity checksum over the
//        stage payload, so truncated or corrupted artifacts fail fast with
//        an explicit diagnostic instead of an obscure parse error deep
//        inside a specific stage's loader.
//
// Compatibility policy: `write_workflow` always emits the current schema.
// `read_workflow` accepts any schema in [kMinLoadableSchemaVersion,
// kCurrentSchemaVersion] and transparently upgrades legacy v1/v2 artifacts
// on load; anything outside that range fails with an explicit
// version-mismatch diagnostic rather than a generic parse failure.
inline constexpr std::int64_t kCurrentSchemaVersion = 3;
inline constexpr std::int64_t kMinLoadableSchemaVersion = 1;

void write_workflow(std::ostream& out, const Workflow& workflow);
[[nodiscard]] Workflow read_workflow(std::istream& in);

} // namespace pluss::ai::nodeb::io
