#pragma once

#include "pluss/nn.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::table {
class DataFrame;
} // namespace pluss::table

namespace pluss::ai::ice {
class Source;
struct BatchPlan;
}

namespace pluss::ai::ice::data {

enum class Layout {
  CrossSection,
  TimeSeries,
  Panel,
  Sequence,
  Trajectory,
};

enum class Role {
  StaticContinuous,
  StaticCategorical,
  KnownContinuous,
  KnownCategorical,
  ObservedContinuous,
  ObservedCategorical,
  FeatureContinuous,
  FeatureCategorical,
};

struct FieldSpec {
  std::string name;
  Role role;
};

struct WindowSpec {
  std::int64_t lookback = 0;
  std::int64_t horizon = 0;
};

struct CategoricalVariable {
  std::int64_t variable_index = 0;
  std::int64_t cardinality = 0;
};

struct CategoricalVocabulary {
  std::string variable_name;
  std::vector<std::string> categories;
};

struct BatchField {
  std::string name;
  Series values = pluss::series::undefined();
  std::vector<CategoricalVariable> categoricals{};
};

namespace field_names {

inline constexpr std::string_view features = "features";
inline constexpr std::string_view static_inputs = "static_inputs";
inline constexpr std::string_view encoder_inputs = "encoder_inputs";
inline constexpr std::string_view decoder_inputs = "decoder_inputs";
inline constexpr std::string_view encoder_target = "encoder_target";

} // namespace field_names

class Batch {
public:
  explicit Batch(Layout layout = Layout::CrossSection) noexcept : layout_(layout) {}

  [[nodiscard]] Layout layout() const noexcept { return layout_; }
  [[nodiscard]] bool has(std::string_view name) const noexcept;
  [[nodiscard]] const Series& field(std::string_view name) const;
  [[nodiscard]] Series& field(std::string_view name);
  [[nodiscard]] const std::vector<CategoricalVariable>& categoricals(std::string_view name) const;
  [[nodiscard]] const std::vector<BatchField>& fields() const noexcept { return fields_; }
  [[nodiscard]] const Series& target() const noexcept { return target_; }
  [[nodiscard]] std::int64_t size() const noexcept;
  [[nodiscard]] std::int64_t feature_width(std::string_view name) const noexcept;

  void set(std::string name, Series values, std::vector<CategoricalVariable> categoricals = {});
  void set_target(Series target) { target_ = std::move(target); }

private:
  Layout layout_{Layout::CrossSection};
  std::vector<BatchField> fields_{};
  Series target_ = pluss::series::undefined();
};

struct DataSpec {
  Layout layout = Layout::CrossSection;
  std::string time_key{};
  std::vector<std::string> group_keys{};
  std::string target{};
  std::vector<FieldSpec> fields{};
  WindowSpec window{};
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
};

class DataSet {
public:
  [[nodiscard]] static DataSet from_frame(const pluss::table::DataFrame& df, const DataSpec& spec);
  [[nodiscard]] static DataSet from_frame(const pluss::table::DataFrame& df,
                                          const DataSpec& spec,
                                          const DataSet& vocabularies_from);
  [[nodiscard]] static DataSet from_batch(Batch batch,
                                          std::vector<CategoricalVocabulary> vocabularies = {});

  [[nodiscard]] const Batch& batch() const noexcept { return batch_; }
  [[nodiscard]] std::int64_t size() const noexcept;
  [[nodiscard]] std::unique_ptr<pluss::ai::ice::Source> source() const;
  [[nodiscard]] std::unique_ptr<pluss::ai::ice::Source> source(const pluss::ai::ice::BatchPlan& learn_plan,
                                                                const pluss::ai::ice::BatchPlan& eval_plan) const;
  [[nodiscard]] const std::vector<CategoricalVocabulary>& vocabularies() const noexcept {
    return vocabularies_;
  }

private:
  Batch batch_{};
  std::vector<CategoricalVocabulary> vocabularies_{};
};

} // namespace pluss::ai::ice::data
