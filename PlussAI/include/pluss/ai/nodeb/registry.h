#pragma once

#include "pluss/ai/nodeb/workflow.h"

#include <functional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pluss::ai::nodeb::registry {

// Zero-argument constructor for a stage in its default configuration.
// `IStage::load_state_lines` is responsible for restoring the stage's
// actual options and learned state on top of that default afterward.
using StageFactory = std::function<Stage()>;

// Thread-safe catalog mapping `IStage::name()` to a factory. The eight
// built-in stages (rule_tokenizer, sentencizer, lemmatizer, matcher,
// tagger, syntax, ner, textcat) are registered lazily on first use.
// Callers may register additional stage types so `Workflow::load` and
// other name-driven construction can resolve them alongside the
// built-ins, without NodeB needing to know about them ahead of time.
class StageRegistry {
public:
  [[nodiscard]] static StageRegistry& instance();

  // Registers `factory` under `name`. Returns false without throwing if
  // `name` is already registered, so callers can decide whether a
  // collision is an error in their context.
  bool add(std::string_view name, StageFactory factory);

  [[nodiscard]] bool contains(std::string_view name) const;
  [[nodiscard]] Stage create(std::string_view name) const;
  [[nodiscard]] std::vector<std::string> names() const;

  StageRegistry(const StageRegistry&) = delete;
  StageRegistry& operator=(const StageRegistry&) = delete;

private:
  StageRegistry();

  mutable std::shared_mutex mutex_{};
  std::unordered_map<std::string, StageFactory> factories_{};
};

} // namespace pluss::ai::nodeb::registry
