#include "pluss/ai/ice/data/dataset.h"
#include "pluss/ai/ice/source.h"

#include "pluss/tils.h"
#include "table/DataFrame.h"

#include <cmath>
#include <unordered_map>
#include <utility>

namespace pluss::ai::ice::data {

namespace {

[[nodiscard]] const BatchField* find_field_(const std::vector<BatchField>& fields, std::string_view name) noexcept {
  for (const auto& field : fields) {
    if (field.name == name) return &field;
  }
  return nullptr;
}

[[nodiscard]] BatchField* find_field_(std::vector<BatchField>& fields, std::string_view name) noexcept {
  for (auto& field : fields) {
    if (field.name == name) return &field;
  }
  return nullptr;
}

[[nodiscard]] std::vector<std::string> raw_keys_(const pluss::table::DataFrame& df, const std::string& name) {
  const auto slate = df.features(name);
  const auto n = slate.size();
  std::vector<std::string> out{};
  out.reserve(static_cast<std::size_t>(n));
  if (slate.kind() == pluss::table::SlateKind::Note) {
    const pluss::Note& note = slate.as_note();
    for (std::int64_t i = 0; i < n; ++i) out.emplace_back(note.at(i));
  } else {
    const auto values = slate.as_series()
                            .to(pluss::pCPU, ScalarType::Float64)
                            .reshape({n})
                            .host<double>();
    for (const double v : values) {
      out.push_back(std::to_string(static_cast<std::int64_t>(std::llround(v))));
    }
  }
  return out;
}

[[nodiscard]] CategoricalVocabulary fit_vocabulary_(const std::string& name,
                                                    const std::vector<std::string>& keys) {
  CategoricalVocabulary vocab{};
  vocab.variable_name = name;
  std::unordered_map<std::string, std::int64_t> seen{};
  for (const auto& key : keys) {
    if (seen.emplace(key, static_cast<std::int64_t>(vocab.categories.size())).second) {
      vocab.categories.push_back(key);
    }
  }
  vocab.categories.emplace_back("__unknown__");
  return vocab;
}

[[nodiscard]] std::vector<std::int64_t> encode_with_vocabulary_(const CategoricalVocabulary& vocab,
                                                                const std::vector<std::string>& keys) {
  std::unordered_map<std::string, std::int64_t> index{};
  index.reserve(vocab.categories.size());
  for (std::size_t i = 0; i < vocab.categories.size(); ++i) {
    index.emplace(vocab.categories[i], static_cast<std::int64_t>(i));
  }
  const auto unknown_code = static_cast<std::int64_t>(vocab.categories.size()) - 1;
  std::vector<std::int64_t> out(keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    const auto it = index.find(keys[i]);
    out[i] = (it != index.end()) ? it->second : unknown_code;
  }
  return out;
}

[[nodiscard]] const CategoricalVocabulary* find_vocabulary_(
    const std::vector<CategoricalVocabulary>& vocabs,
    const std::string& name) {
  for (const auto& vocab : vocabs) {
    if (vocab.variable_name == name) return &vocab;
  }
  return nullptr;
}

[[nodiscard]] bool is_categorical_(Role role) {
  switch (role) {
    case Role::StaticCategorical:
    case Role::KnownCategorical:
    case Role::ObservedCategorical:
    case Role::FeatureCategorical:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool is_cross_section_feature_(Role role) {
  return role == Role::FeatureContinuous || role == Role::FeatureCategorical;
}

[[nodiscard]] std::vector<std::string> group_keys_(const pluss::table::DataFrame& df,
                                                   const std::vector<std::string>& group_keys,
                                                   std::int64_t n) {
  if (group_keys.empty()) return std::vector<std::string>(static_cast<std::size_t>(n));
  std::vector<std::vector<std::string>> per_column{};
  per_column.reserve(group_keys.size());
  for (const auto& col : group_keys) {
    per_column.push_back(raw_keys_(df, col));
  }

  std::vector<std::string> out(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    std::string key{};
    for (const auto& values : per_column) {
      key += values[static_cast<std::size_t>(i)];
      key += '\x1f';
    }
    out[static_cast<std::size_t>(i)] = std::move(key);
  }
  return out;
}

[[nodiscard]] std::vector<std::int64_t> window_starts_(const std::vector<std::string>& group_key,
                                                       const std::vector<std::int64_t>& time_idx,
                                                       std::int64_t window_length) {
  const auto n = static_cast<std::int64_t>(group_key.size());
  std::vector<std::int64_t> starts{};
  std::int64_t run_start = 0;
  for (std::int64_t i = 1; i <= n; ++i) {
    const bool run_broken =
        (i == n) ||
        group_key[static_cast<std::size_t>(i)] != group_key[static_cast<std::size_t>(i - 1)] ||
        time_idx[static_cast<std::size_t>(i)] != time_idx[static_cast<std::size_t>(i - 1)] + 1;
    if (run_broken) {
      const std::int64_t run_length = i - run_start;
      for (std::int64_t s = run_start; s + window_length <= run_start + run_length; ++s) {
        starts.push_back(s);
      }
      run_start = i;
    }
  }
  return starts;
}

[[nodiscard]] Series gather_index_(const std::vector<std::int64_t>& window_starts,
                                   std::int64_t offset,
                                   std::int64_t steps,
                                   Platform platform) {
  std::vector<std::int64_t> flat(window_starts.size() * static_cast<std::size_t>(steps));
  for (std::size_t w = 0; w < window_starts.size(); ++w) {
    for (std::int64_t s = 0; s < steps; ++s) {
      flat[w * static_cast<std::size_t>(steps) + static_cast<std::size_t>(s)] =
          window_starts[w] + offset + s;
    }
  }
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  return pluss::make_series(flat, {static_cast<std::int64_t>(flat.size())}, icfg).to(platform);
}

[[nodiscard]] Series materialize_(const Series& column,
                                  const Series& index,
                                  std::int64_t num_windows,
                                  std::int64_t steps) {
  const Series flat = column.index_select(0, index);
  if (steps == 1) return flat.reshape({num_windows});
  return flat.reshape({num_windows, steps});
}

struct PreparedField {
  const FieldSpec* spec = nullptr;
  Series column = pluss::series::undefined();
};

[[nodiscard]] std::vector<PreparedField> prepare_fields_(
    const pluss::table::DataFrame& df,
    const std::vector<const FieldSpec*>& fields,
    const DataSpec& spec,
    const DataSet* reuse_from,
    std::vector<CategoricalVocabulary>& vocabularies_out) {
  std::vector<PreparedField> out{};
  out.reserve(fields.size());
  for (const auto* field : fields) {
    PreparedField prepared{};
    prepared.spec = field;
    if (is_categorical_(field->role)) {
      const auto keys = raw_keys_(df, field->name);
      CategoricalVocabulary vocab{};
      if (reuse_from != nullptr) {
        const auto* existing = find_vocabulary_(reuse_from->vocabularies(), field->name);
        PLUSS_CHECK(
            existing != nullptr,
            "pluss::ai::ice::data::DataSet::from_frame: no fitted vocabulary for categorical field '" +
                field->name + "' in vocabularies_from.");
        vocab = *existing;
      } else {
        vocab = fit_vocabulary_(field->name, keys);
      }
      const auto codes = encode_with_vocabulary_(vocab, keys);
      const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
      prepared.column =
          pluss::make_series(codes, {static_cast<std::int64_t>(codes.size())}, icfg)
              .to(spec.cfg.platform, spec.cfg.dtype);
      vocabularies_out.push_back(std::move(vocab));
    } else {
      const auto n = df.features(field->name).size();
      prepared.column =
          df.features(field->name).as_series().to(spec.cfg.platform, spec.cfg.dtype).reshape({n});
    }
    out.push_back(std::move(prepared));
  }
  return out;
}

[[nodiscard]] std::vector<CategoricalVariable> categoricals_for_prepared_(
    const std::vector<PreparedField>& prepared,
    const std::vector<CategoricalVocabulary>& vocabularies) {
  std::vector<CategoricalVariable> out{};
  for (std::size_t i = 0; i < prepared.size(); ++i) {
    if (!is_categorical_(prepared[i].spec->role)) continue;
    const auto* vocab = find_vocabulary_(vocabularies, prepared[i].spec->name);
    PLUSS_CHECK(vocab != nullptr,
                "pluss::ai::ice::data::DataSet: internal error, missing vocabulary.");
    out.push_back(CategoricalVariable{
        .variable_index = static_cast<std::int64_t>(i),
        .cardinality = static_cast<std::int64_t>(vocab->categories.size()),
    });
  }
  return out;
}

[[nodiscard]] Batch build_cross_section_(const pluss::table::DataFrame& df,
                                         const DataSpec& spec,
                                         const DataSet* reuse_from,
                                         std::vector<CategoricalVocabulary>& vocabularies_out) {
  PLUSS_CHECK(!spec.fields.empty(),
              "pluss::ai::ice::data::DataSet::from_frame: CrossSection layout requires fields.");
  std::vector<const FieldSpec*> feature_fields{};
  feature_fields.reserve(spec.fields.size());
  for (const auto& field : spec.fields) {
    PLUSS_CHECK(
        is_cross_section_feature_(field.role),
        "pluss::ai::ice::data::DataSet::from_frame: CrossSection layout only accepts "
        "FeatureContinuous and FeatureCategorical roles.");
    feature_fields.push_back(&field);
  }

  const auto prepared = prepare_fields_(df, feature_fields, spec, reuse_from, vocabularies_out);
  std::vector<Series> columns{};
  columns.reserve(prepared.size());
  for (const auto& field : prepared) {
    columns.push_back(field.column);
  }

  Batch batch(Layout::CrossSection);
  batch.set(
      std::string(field_names::features),
      pluss::series::pile(columns, -1),
      categoricals_for_prepared_(prepared, vocabularies_out));
  if (!spec.target.empty()) {
    const auto n = df.features(spec.target).size();
    batch.set_target(
        df.features(spec.target).as_series().to(spec.cfg.platform, spec.cfg.dtype).reshape({n}));
  }
  return batch;
}

[[nodiscard]] Batch build_panel_(const pluss::table::DataFrame& df,
                                 const DataSpec& spec,
                                 const DataSet* reuse_from,
                                 std::vector<CategoricalVocabulary>& vocabularies_out) {
  PLUSS_CHECK(!spec.time_key.empty(),
              "pluss::ai::ice::data::DataSet::from_frame: Panel/TimeSeries layout requires time_key.");
  PLUSS_CHECK(!spec.target.empty(),
              "pluss::ai::ice::data::DataSet::from_frame: Panel/TimeSeries layout requires target.");
  PLUSS_CHECK(spec.window.lookback > 0 && spec.window.horizon > 0,
              "pluss::ai::ice::data::DataSet::from_frame: Panel/TimeSeries layout requires "
              "positive window.lookback and window.horizon.");

  std::vector<std::string_view> sort_columns{};
  sort_columns.reserve(spec.group_keys.size() + 1);
  for (const auto& col : spec.group_keys) sort_columns.push_back(col);
  sort_columns.push_back(spec.time_key);
  const pluss::table::DataFrame sorted =
      df.arrange(pluss::msharp::RangeView::Of<std::string_view>(sort_columns));

  const auto n = sorted.features(spec.time_key).size();
  const std::vector<std::int64_t> time_idx = sorted.features(spec.time_key)
                            .as_series()
                            .to(pluss::pCPU, ScalarType::Int64)
                            .reshape({n})
                            .host<std::int64_t>();
  const auto group_key = group_keys_(sorted, spec.group_keys, n);
  const std::int64_t window_length = spec.window.lookback + spec.window.horizon;
  const auto starts = window_starts_(group_key, time_idx, window_length);
  PLUSS_CHECK(!starts.empty(),
              "pluss::ai::ice::data::DataSet::from_frame: no group has enough contiguous rows "
              "(lookback + horizon) to form a single window.");

  std::vector<const FieldSpec*> static_fields{};
  std::vector<const FieldSpec*> encoder_fields{};
  std::vector<const FieldSpec*> decoder_fields{};
  for (const auto& field : spec.fields) {
    switch (field.role) {
      case Role::StaticContinuous:
      case Role::StaticCategorical:
        static_fields.push_back(&field);
        break;
      case Role::KnownContinuous:
      case Role::KnownCategorical:
        encoder_fields.push_back(&field);
        decoder_fields.push_back(&field);
        break;
      case Role::ObservedContinuous:
      case Role::ObservedCategorical:
        encoder_fields.push_back(&field);
        break;
      default:
        PLUSS_FAIL(
            "pluss::ai::ice::data::DataSet::from_frame: Panel/TimeSeries layout only accepts "
            "Static*, Known*, and Observed* field roles.");
    }
  }

  const auto static_prepared =
      prepare_fields_(sorted, static_fields, spec, reuse_from, vocabularies_out);
  const auto encoder_prepared =
      prepare_fields_(sorted, encoder_fields, spec, reuse_from, vocabularies_out);
  const auto decoder_prepared =
      prepare_fields_(sorted, decoder_fields, spec, reuse_from, vocabularies_out);

  const auto num_windows = static_cast<std::int64_t>(starts.size());
  Batch batch(spec.layout);
  if (!static_prepared.empty()) {
    const Series index = gather_index_(starts, 0, 1, spec.cfg.platform);
    std::vector<Series> fields{};
    fields.reserve(static_prepared.size());
    for (const auto& field : static_prepared) {
      fields.push_back(materialize_(field.column, index, num_windows, 1));
    }
    batch.set(
        std::string(field_names::static_inputs),
        pluss::series::pile(fields, -1),
        categoricals_for_prepared_(static_prepared, vocabularies_out));
  }

  if (!encoder_prepared.empty()) {
    const Series index = gather_index_(starts, 0, spec.window.lookback, spec.cfg.platform);
    std::vector<Series> fields{};
    fields.reserve(encoder_prepared.size());
    for (const auto& field : encoder_prepared) {
      fields.push_back(materialize_(field.column, index, num_windows, spec.window.lookback));
    }
    batch.set(
        std::string(field_names::encoder_inputs),
        pluss::series::pile(fields, -1),
        categoricals_for_prepared_(encoder_prepared, vocabularies_out));
  }

  if (!decoder_prepared.empty()) {
    const Series index =
        gather_index_(starts, spec.window.lookback, spec.window.horizon, spec.cfg.platform);
    std::vector<Series> fields{};
    fields.reserve(decoder_prepared.size());
    for (const auto& field : decoder_prepared) {
      fields.push_back(materialize_(field.column, index, num_windows, spec.window.horizon));
    }
    batch.set(
        std::string(field_names::decoder_inputs),
        pluss::series::pile(fields, -1),
        categoricals_for_prepared_(decoder_prepared, vocabularies_out));
  }

  const Series encoder_target_index =
      gather_index_(starts, 0, spec.window.lookback, spec.cfg.platform);
  const Series target_index =
      gather_index_(starts, spec.window.lookback, spec.window.horizon, spec.cfg.platform);
  const Series target_column =
      sorted.features(spec.target).as_series().to(spec.cfg.platform, spec.cfg.dtype).reshape({n});
  batch.set(
      std::string(field_names::encoder_target),
      materialize_(target_column, encoder_target_index, num_windows, spec.window.lookback));
  // The target keeps its decoder axis even for a one-step horizon: [N, horizon],
  // matching the forecaster contract, instead of squeezing to [N].
  batch.set_target(target_column.index_select(0, target_index).reshape({num_windows, spec.window.horizon}));
  return batch;
}

} // namespace

bool Batch::has(std::string_view name) const noexcept {
  return find_field_(fields_, name) != nullptr;
}

const Series& Batch::field(std::string_view name) const {
  const auto* out = find_field_(fields_, name);
  PLUSS_CHECK(out != nullptr,
              "pluss::ai::ice::data::Batch::field: no field named '" + std::string(name) + "'.");
  return out->values;
}

Series& Batch::field(std::string_view name) {
  auto* out = find_field_(fields_, name);
  PLUSS_CHECK(out != nullptr,
              "pluss::ai::ice::data::Batch::field: no field named '" + std::string(name) + "'.");
  return out->values;
}

const std::vector<CategoricalVariable>& Batch::categoricals(std::string_view name) const {
  const auto* out = find_field_(fields_, name);
  PLUSS_CHECK(
      out != nullptr,
      "pluss::ai::ice::data::Batch::categoricals: no field named '" + std::string(name) + "'.");
  return out->categoricals;
}

std::int64_t Batch::size() const noexcept {
  if (!fields_.empty() && fields_.front().values.defined() &&
      !fields_.front().values.sizes().empty()) {
    return fields_.front().values.sizes()[0];
  }
  if (target_.defined() && !target_.sizes().empty()) return target_.sizes()[0];
  return 0;
}

std::int64_t Batch::feature_width(std::string_view name) const noexcept {
  const auto* out = find_field_(fields_, name);
  if (out == nullptr || !out->values.defined() || out->values.sizes().empty()) return 0;
  return out->values.sizes().back();
}

void Batch::set(std::string name, Series values, std::vector<CategoricalVariable> categoricals) {
  if (auto* existing = find_field_(fields_, name); existing != nullptr) {
    existing->values = std::move(values);
    existing->categoricals = std::move(categoricals);
    return;
  }
  fields_.push_back(BatchField{
      .name = std::move(name),
      .values = std::move(values),
      .categoricals = std::move(categoricals),
  });
}

DataSet DataSet::from_frame(const pluss::table::DataFrame& df, const DataSpec& spec) {
  return from_frame(df, spec, DataSet{});
}

DataSet DataSet::from_frame(const pluss::table::DataFrame& df,
                            const DataSpec& spec,
                            const DataSet& vocabularies_from) {
  PLUSS_CALL_GUARD();

  DataSet out{};
  const DataSet* reuse_from = vocabularies_from.vocabularies_.empty() ? nullptr : &vocabularies_from;
  switch (spec.layout) {
    case Layout::CrossSection:
      out.batch_ = build_cross_section_(df, spec, reuse_from, out.vocabularies_);
      break;
    case Layout::TimeSeries:
    case Layout::Panel:
      out.batch_ = build_panel_(df, spec, reuse_from, out.vocabularies_);
      break;
    case Layout::Sequence:
    case Layout::Trajectory:
      PLUSS_FAIL(
          "pluss::ai::ice::data::DataSet::from_frame: Sequence and Trajectory builders are not "
          "implemented yet.");
  }
  return out;
}

DataSet DataSet::from_batch(Batch batch, std::vector<CategoricalVocabulary> vocabularies) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(batch.size() > 0,
              "pluss::ai::ice::data::DataSet::from_batch: batch must contain at least one row.");
  DataSet out{};
  out.batch_ = std::move(batch);
  out.vocabularies_ = std::move(vocabularies);
  return out;
}

std::int64_t DataSet::size() const noexcept { return batch_.size(); }

std::unique_ptr<pluss::ai::ice::Source> DataSet::source() const {
  PLUSS_CALL_GUARD();
  return source(pluss::ai::ice::BatchPlan{}, pluss::ai::ice::BatchPlan{});
}

std::unique_ptr<pluss::ai::ice::Source> DataSet::source(
    const pluss::ai::ice::BatchPlan& learn_plan,
    const pluss::ai::ice::BatchPlan& eval_plan) const {
  PLUSS_CALL_GUARD();
  // A single DataSet does not carry a semantic train/valid/test split.
  // The default producer therefore exposes the same materialized batch for
  // learn/test/infer so callers can plug it straight into Coach for quick
  // experiments, while more explicit multi-split flows can still construct
  // DataSetSource directly.
  return std::make_unique<pluss::ai::ice::DataSetSource>(
      *this,
      std::nullopt,
      *this,
      *this,
      learn_plan,
      eval_plan);
}

} // namespace pluss::ai::ice::data
