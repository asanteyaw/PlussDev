#include "pluss/ai/nodeb/registry.h"

#include "pluss/ai/nodeb/tasks.h"
#include "pluss/ai/nodeb/tokenize.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace pluss::ai::nodeb::registry {

StageRegistry::StageRegistry() {
  factories_.reserve(8);
  factories_.emplace("rule_tokenizer", [] { return stage::tokenize(); });
  factories_.emplace("sentencizer", [] { return stage::sentencize(); });
  factories_.emplace("lemmatizer", [] { return stage::lemmatize(); });
  factories_.emplace("matcher", [] { return stage::matcher(); });
  factories_.emplace("tagger", [] { return stage::tagger(); });
  factories_.emplace("syntax", [] { return stage::syntax(); });
  factories_.emplace("ner", [] { return stage::ner(); });
  factories_.emplace("textcat", [] { return stage::textcat(); });
}

StageRegistry& StageRegistry::instance() {
  static StageRegistry singleton;
  return singleton;
}

bool StageRegistry::add(std::string_view name, StageFactory factory) {
  PLUSS_CALL_GUARD();
  if (name.empty() || !factory) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::registry::StageRegistry::add: a non-empty name and a valid factory are required.");
  }
  std::unique_lock lock(mutex_);
  return factories_.try_emplace(std::string(name), std::move(factory)).second;
}

bool StageRegistry::contains(std::string_view name) const {
  std::shared_lock lock(mutex_);
  return factories_.contains(std::string(name));
}

Stage StageRegistry::create(std::string_view name) const {
  PLUSS_CALL_GUARD();
  StageFactory factory;
  {
    std::shared_lock lock(mutex_);
    const auto it = factories_.find(std::string(name));
    if (it == factories_.end()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::registry::StageRegistry::create: unknown stage '" + std::string(name) + "'.");
    }
    factory = it->second;
  }
  return factory();
}

std::vector<std::string> StageRegistry::names() const {
  std::shared_lock lock(mutex_);
  std::vector<std::string> out{};
  out.reserve(factories_.size());
  std::ranges::transform(factories_, std::back_inserter(out),
                         [](const auto& entry) { return entry.first; });
  std::ranges::sort(out);
  return out;
}

} // namespace pluss::ai::nodeb::registry
