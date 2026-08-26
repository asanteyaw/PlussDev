#pragma once

#include "pluss/io.h"
#include "pluss/ai/nodej/io.h"
#include "pluss/ai/lymph/api.h"

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pluss::io {

inline void save(
    const ::pluss::ai::nodej::Flow& flow,
    std::string_view path,
    const ::pluss::ai::nodej::LabeledBatch& train_batch,
    const ::pluss::ai::nodej::TrainOptions& train_options = {},
    ::pluss::ai::nodej::io::FlowBundleMetadata metadata = {}) {
  const auto status = ::pluss::ai::nodej::io::save(
      std::string(path),
      flow,
      train_batch,
      train_options,
      std::move(metadata));
  PLUSS_CHECK(
      status.ok(),
      "pluss::io::save(nodej::Flow): " + status.message);
}

inline void save(
    const ::pluss::ai::lymph::models::Model& model,
    std::string_view path,
    const ::pluss::ai::nodej::LabeledBatch& train_batch,
    const ::pluss::ai::nodej::TrainOptions& train_options = {},
    ::pluss::ai::nodej::io::FlowBundleMetadata metadata = {}) {
  model.save(std::string(path), train_batch, train_options, std::move(metadata));
}

inline void warn_model_load_options_ignored_(
    const ::pluss::io::LoadOptions& options,
    std::string_view api_name) {
  if (!options.target_platform.has_value() && !options.preserve_saved_platform && !options.strict_platform) {
    return;
  }
  PLUSS_WARN(
      "pluss::io::" + std::string(api_name)
      + ": LoadOptions target platform flags are ignored for model/workflow artifacts");
}

template <class T>
  requires std::is_same_v<std::remove_cvref_t<T>, ::pluss::ai::nodej::io::FlowBundle>
inline T load(std::string_view path, const ::pluss::io::LoadOptions& options = {}) {
  warn_model_load_options_ignored_(options, "load<nodej::io::FlowBundle>");
  auto out = ::pluss::ai::nodej::io::load(std::string(path));
  PLUSS_CHECK(
      out.ok(),
      "pluss::io::load<nodej::io::FlowBundle>: " + out.status().message);
  return out.move_value();
}

template <class T>
  requires std::is_same_v<std::remove_cvref_t<T>, ::pluss::ai::nodej::Flow>
inline T load(std::string_view path, const ::pluss::io::LoadOptions& options = {}) {
  warn_model_load_options_ignored_(options, "load<nodej::Flow>");
  auto out = ::pluss::ai::nodej::io::load_and_restore(std::string(path));
  PLUSS_CHECK(
      out.ok(),
      "pluss::io::load<nodej::Flow>: " + out.status().message);
  return out.move_value();
}

template <class T>
  requires std::is_same_v<std::remove_cvref_t<T>, ::pluss::ai::lymph::models::Model>
inline T load(std::string_view path, const ::pluss::io::LoadOptions& options = {}) {
  warn_model_load_options_ignored_(options, "load<lymph::models::Model>");
  return ::pluss::ai::lymph::models::Model::load(std::string(path));
}

inline void load(
    ::pluss::ai::nodej::io::FlowBundle& out,
    std::string_view path,
    const ::pluss::io::LoadOptions& options = {}) {
  out = load<::pluss::ai::nodej::io::FlowBundle>(path, options);
}

inline void load(
    ::pluss::ai::nodej::Flow& out,
    std::string_view path,
    const ::pluss::io::LoadOptions& options = {}) {
  out = load<::pluss::ai::nodej::Flow>(path, options);
}

inline void load(
    ::pluss::ai::lymph::models::Model& out,
    std::string_view path,
    const ::pluss::io::LoadOptions& options = {}) {
  out = load<::pluss::ai::lymph::models::Model>(path, options);
}

} // namespace pluss::io

namespace pluss {
using io::load;
using io::save;
} // namespace pluss
