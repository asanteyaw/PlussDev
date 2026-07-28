#include "pluss/ai/lymph/prep.h"

namespace pluss::ai::lymph {

[[nodiscard]] std::vector<std::string> prep_resolve_series_feature_names_(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  if (feature_names_pinned) {
    for (const auto& name : pinned_feature_names) {
      if (!frame.get(name).has_value()) {
        ::pluss::throw_error(std::string(where) + ": missing feature '" + name + "' in input DataFrame.");
      }
    }
    return pinned_feature_names;
  }
  const auto names = frame.feataxis().series().features();
  if (names.empty()) {
    ::pluss::throw_error(std::string(where) + ": no Series-backed features found in input DataFrame.");
  }
  return names;
}

[[nodiscard]] DataFrameBatchOptions prep_batch_options_for_(pluss::msharp::RangeView::Of<std::string> names) {
  DataFrameBatchOptions options{};
  options.feature_names = copy_range_values(names);
  options.layout = ObservationLayout::Rows;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
  options.note_policy = DataNotePolicy::Reject;
  return options;
}

template <class WrapperT>
[[nodiscard]] std::vector<std::string> prep_resolve_feature_names_for_(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  return prep_resolve_series_feature_names_(frame, pinned_feature_names, feature_names_pinned, where);
}

template <>
[[nodiscard]] std::vector<std::string> prep_resolve_feature_names_for_<prep::OneHotEncoder>(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  (void)frame;
  if (!feature_names_pinned || pinned_feature_names.empty()) {
    ::pluss::throw_error(std::string(where) +
                             ": explicit categorical feature selection is required; call .features({\"name\", ...}) "
                             "before learn()/map()/learn_map() on DataFrame.");
  }
  return pinned_feature_names;
}

template <>
[[nodiscard]] std::vector<std::string> prep_resolve_feature_names_for_<prep::FrequencyEncoder>(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  (void)frame;
  if (!feature_names_pinned || pinned_feature_names.empty()) {
    ::pluss::throw_error(std::string(where) +
                             ": explicit categorical feature selection is required; call .features({\"name\", ...}) "
                             "before learn()/map()/learn_map() on DataFrame.");
  }
  return pinned_feature_names;
}

template <>
[[nodiscard]] std::vector<std::string> prep_resolve_feature_names_for_<prep::OrdinalEncoder>(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  (void)frame;
  if (!feature_names_pinned || pinned_feature_names.empty()) {
    ::pluss::throw_error(std::string(where) +
                             ": explicit categorical feature selection is required; call .features({\"name\", ...}) "
                             "before learn()/map()/learn_map() on DataFrame.");
  }
  return pinned_feature_names;
}

template <>
[[nodiscard]] std::vector<std::string> prep_resolve_feature_names_for_<prep::LabelEncoder>(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  (void)frame;
  if (!feature_names_pinned || pinned_feature_names.empty()) {
    ::pluss::throw_error(std::string(where) +
                             ": explicit categorical feature selection is required; call .features({\"name\", ...}) "
                             "before learn()/map()/learn_map() on DataFrame.");
  }
  return pinned_feature_names;
}

template <class WrapperT>
[[nodiscard]] DataFrameBatchOptions prep_batch_options_for_wrapper_(pluss::msharp::RangeView::Of<std::string> names) {
  return prep_batch_options_for_(names);
}

template <>
[[nodiscard]] DataFrameBatchOptions prep_batch_options_for_wrapper_<prep::OneHotEncoder>(
    pluss::msharp::RangeView::Of<std::string> names) {
  auto options = prep_batch_options_for_(names);
  options.note_policy = DataNotePolicy::Hash;
  return options;
}

template <>
[[nodiscard]] DataFrameBatchOptions prep_batch_options_for_wrapper_<prep::FrequencyEncoder>(
    pluss::msharp::RangeView::Of<std::string> names) {
  auto options = prep_batch_options_for_(names);
  options.note_policy = DataNotePolicy::Hash;
  return options;
}

template <>
[[nodiscard]] DataFrameBatchOptions prep_batch_options_for_wrapper_<prep::OrdinalEncoder>(
    pluss::msharp::RangeView::Of<std::string> names) {
  auto options = prep_batch_options_for_(names);
  options.note_policy = DataNotePolicy::Hash;
  return options;
}

template <>
[[nodiscard]] DataFrameBatchOptions prep_batch_options_for_wrapper_<prep::LabelEncoder>(
    pluss::msharp::RangeView::Of<std::string> names) {
  auto options = prep_batch_options_for_(names);
  options.note_policy = DataNotePolicy::Hash;
  return options;
}

[[nodiscard]] pluss::table::DataFrame prep_merge_mapped_(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<std::string> names,
    const Series& mapped,
    const char* where) {
  if (mapped.dim() != 2) {
    ::pluss::throw_error(std::string(where) + ": mapped feature matrix must be rank-2.");
  }
  const auto sizes = mapped.sizes();
  if (sizes.size() != 2) {
    ::pluss::throw_error(std::string(where) + ": mapped feature matrix must have two dimensions.");
  }
  const std::int64_t sample_count = sizes[0];
  const std::int64_t feature_count = sizes[1];
  if (sample_count != frame.ncases()) {
    ::pluss::throw_error(std::string(where) + ": mapped sample count mismatch.");
  }
  if (feature_count != static_cast<std::int64_t>(names.size())) {
    std::vector<std::pair<std::string, pluss::table::Thread>> columns{};
    columns.reserve(static_cast<std::size_t>(feature_count));
    for (std::int64_t j = 0; j < feature_count; ++j) {
      const Series idx = pluss::make_series(
          std::vector<std::int64_t>{j},
          {1},
          Config{}.with_dtype(ScalarType::Int64).with_platform(mapped.platform()));
      const Series col = mapped.index_select(1, idx).reshape({sample_count});
      std::string out_name;
      if (names.size() == 1) {
        out_name = names.front() + "_" + std::to_string(j);
      } else {
        out_name = std::string("feature_") + std::to_string(j);
      }
      columns.emplace_back(std::move(out_name), pluss::table::Thread(col));
    }
    return pluss::table::DataFrame(std::move(columns), frame.label().values());
  }

  auto out = frame;
  for (std::int64_t j = 0; j < feature_count; ++j) {
    const Series idx = pluss::make_series(
        std::vector<std::int64_t>{j},
        {1},
        Config{}.with_dtype(ScalarType::Int64).with_platform(mapped.platform()));
    const Series col = mapped.index_select(1, idx).reshape({sample_count});
    out.set_(names[static_cast<std::size_t>(j)], pluss::table::Thread(col));
  }
  return out;
}

std::vector<std::string> prep::SimpleImputer::resolved_feature_names_(const pluss::table::DataFrame& frame,
                                                                              const char* where) const {
  return prep_resolve_series_feature_names_(frame, feature_names_, feature_names_pinned_, where);
}

DataFrameBatchOptions prep::SimpleImputer::batch_options_for_(pluss::msharp::RangeView::Of<std::string> names) {
  return prep_batch_options_for_(names);
}

pluss::table::DataFrame prep::SimpleImputer::merge_mapped_(const pluss::table::DataFrame& frame,
                                                                   pluss::msharp::RangeView::Of<std::string> names,
                                                                   const Series& mapped,
                                                                   const char* where) {
  return prep_merge_mapped_(frame, names, mapped, where);
}

void prep::SimpleImputer::refresh_statistics_() {
  statistics_ = Series::undefined();
  if (!learned_) return;
  const auto state = resolve(core_.dump_state(), "prep::SimpleImputer.dump_state");
  if (state.size() >= 2 && state[1].defined()) {
    statistics_ = state[1].reshape({state[1].numel()});
  }
}

std::vector<std::string> prep::StandardScaler::resolved_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) const {
  return prep_resolve_series_feature_names_(frame, feature_names_, feature_names_pinned_, where);
}

DataFrameBatchOptions prep::StandardScaler::batch_options_for_(pluss::msharp::RangeView::Of<std::string> names) {
  return prep_batch_options_for_(names);
}

pluss::table::DataFrame prep::StandardScaler::merge_mapped_(const pluss::table::DataFrame& frame,
                                                                    pluss::msharp::RangeView::Of<std::string> names,
                                                                    const Series& mapped,
                                                                    const char* where) {
  return prep_merge_mapped_(frame, names, mapped, where);
}

void prep::StandardScaler::refresh_statistics_() {
  mean_ = Series::undefined();
  scale_ = Series::undefined();
  if (!learned_) return;
  const auto state = resolve(core_.dump_state(), "prep::StandardScaler.dump_state");
  if (state.size() >= 3) {
    if (state[1].defined()) mean_ = state[1].reshape({state[1].numel()});
    if (state[2].defined()) scale_ = state[2].reshape({state[2].numel()});
  }
}

prep::SimpleImputer& prep::SimpleImputer::learn(const pluss::table::DataFrame& frame, const TrainOptions& options) {
  const auto names = resolved_feature_names_(frame, "prep::SimpleImputer.learn");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::SimpleImputer.learn.feature_batch_from_dataframe");
  resolve(core_.learn(batch, options), "prep::SimpleImputer.learn");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return *this;
}

pluss::table::DataFrame prep::SimpleImputer::map(const pluss::table::DataFrame& frame, const InferOptions& options) const {
  if (!learned_) {
    ::pluss::throw_error("prep::SimpleImputer.map: call learn() or learn_map() first.");
  }
  const auto names = resolved_feature_names_(frame, "prep::SimpleImputer.map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::SimpleImputer.map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.map(batch, options), "prep::SimpleImputer.map");
  return merge_mapped_(frame, names, mapped, "prep::SimpleImputer.map");
}

pluss::table::DataFrame prep::SimpleImputer::learn_map(const pluss::table::DataFrame& frame,
                                                               const TrainOptions& learn_options,
                                                               const InferOptions& infer_options) {
  const auto names = resolved_feature_names_(frame, "prep::SimpleImputer.learn_map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::SimpleImputer.learn_map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.learn_map(batch, learn_options, infer_options), "prep::SimpleImputer.learn_map");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return merge_mapped_(frame, names, mapped, "prep::SimpleImputer.learn_map");
}

prep::StandardScaler& prep::StandardScaler::learn(const pluss::table::DataFrame& frame, const TrainOptions& options) {
  const auto names = resolved_feature_names_(frame, "prep::StandardScaler.learn");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::StandardScaler.learn.feature_batch_from_dataframe");
  resolve(core_.learn(batch, options), "prep::StandardScaler.learn");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return *this;
}

pluss::table::DataFrame prep::StandardScaler::map(const pluss::table::DataFrame& frame, const InferOptions& options) const {
  if (!learned_) {
    ::pluss::throw_error("prep::StandardScaler.map: call learn() or learn_map() first.");
  }
  const auto names = resolved_feature_names_(frame, "prep::StandardScaler.map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::StandardScaler.map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.map(batch, options), "prep::StandardScaler.map");
  return merge_mapped_(frame, names, mapped, "prep::StandardScaler.map");
}

pluss::table::DataFrame prep::StandardScaler::learn_map(const pluss::table::DataFrame& frame,
                                                                const TrainOptions& learn_options,
                                                                const InferOptions& infer_options) {
  const auto names = resolved_feature_names_(frame, "prep::StandardScaler.learn_map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::StandardScaler.learn_map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.learn_map(batch, learn_options, infer_options), "prep::StandardScaler.learn_map");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return merge_mapped_(frame, names, mapped, "prep::StandardScaler.learn_map");
}

std::vector<std::string> prep::OutlierIsolator::resolved_feature_names_(const pluss::table::DataFrame& frame,
                                                                                 const char* where) const {
  return prep_resolve_series_feature_names_(frame, feature_names_, feature_names_pinned_, where);
}

DataFrameBatchOptions prep::OutlierIsolator::batch_options_for_(pluss::msharp::RangeView::Of<std::string> names) {
  return prep_batch_options_for_(names);
}

prep::OutlierIsolator& prep::OutlierIsolator::learn(const pluss::table::DataFrame& frame,
                                                            const TrainOptions& options) {
  const auto names = resolved_feature_names_(frame, "prep::OutlierIsolator.learn");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::OutlierIsolator.learn.feature_batch_from_dataframe");
  resolve(core_.learn(batch, options), "prep::OutlierIsolator.learn");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  return *this;
}

pluss::table::Thread prep::OutlierIsolator::infer(const pluss::table::DataFrame& frame,
                                                         const InferOptions& options) const {
  if (!learned_) {
    ::pluss::throw_error("prep::OutlierIsolator.infer: call learn() or learn_infer() first.");
  }
  const auto names = resolved_feature_names_(frame, "prep::OutlierIsolator.infer");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::OutlierIsolator.infer.feature_batch_from_dataframe");
  return pluss::table::Thread(resolve(core_.infer(batch, options), "prep::OutlierIsolator.infer"));
}

pluss::table::Thread prep::OutlierIsolator::score_samples(const pluss::table::DataFrame& frame,
                                                                 const InferOptions& options) const {
  if (!learned_) {
    ::pluss::throw_error("prep::OutlierIsolator.score_samples: call learn() or learn_infer() first.");
  }
  const auto names = resolved_feature_names_(frame, "prep::OutlierIsolator.score_samples");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::OutlierIsolator.score_samples.feature_batch_from_dataframe");
  return pluss::table::Thread(resolve(core_.score_samples(batch, options), "prep::OutlierIsolator.score_samples"));
}

pluss::table::Thread prep::OutlierIsolator::learn_infer(const pluss::table::DataFrame& frame,
                                                               const TrainOptions& learn_options,
                                                               const InferOptions& infer_options) {
  const auto names = resolved_feature_names_(frame, "prep::OutlierIsolator.learn_infer");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::OutlierIsolator.learn_infer.feature_batch_from_dataframe");
  const auto labels = resolve(core_.learn_infer(batch, learn_options, infer_options), "prep::OutlierIsolator.learn_infer");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  return pluss::table::Thread(labels);
}

pluss::table::DataFrame prep::OutlierIsolator::filter_inliers(const pluss::table::DataFrame& frame,
                                                                      const InferOptions& options) const {
  return frame.filter((infer(frame, options) == 1.0));
}

#define PLUSS_NODEJ_DEFINE_PREP_WRAPPER(WrapperName, Label)                                      \
  std::vector<std::string> prep::WrapperName::resolved_feature_names_(                    \
      const pluss::table::DataFrame& frame,                                                       \
      const char* where) const {                                                                  \
    return prep_resolve_feature_names_for_<prep::WrapperName>(                                    \
        frame,                                                                                    \
        feature_names_,                                                                           \
        feature_names_pinned_,                                                                    \
        where);                                                                                   \
  }                                                                                               \
                                                                                                  \
  DataFrameBatchOptions prep::WrapperName::batch_options_for_(pluss::msharp::RangeView::Of<std::string> names) { \
    return prep_batch_options_for_wrapper_<prep::WrapperName>(names);                             \
  }                                                                                               \
                                                                                                  \
  pluss::table::DataFrame prep::WrapperName::merge_mapped_(                                \
      const pluss::table::DataFrame& frame,                                                       \
      pluss::msharp::RangeView::Of<std::string> names,                                                      \
      const Series& mapped,                                                                       \
      const char* where) {                                                                        \
    return prep_merge_mapped_(frame, names, mapped, where);                                       \
  }                                                                                               \
                                                                                                  \
  void prep::WrapperName::refresh_state_() {                                               \
    state_.clear();                                                                               \
    if (!learned_) return;                                                                        \
    try {                                                                                         \
      state_ = resolve(core_.dump_state(), Label ".dump_state");                                  \
    } catch (...) {                                                                               \
      state_.clear();                                                                             \
    }                                                                                             \
    prep_refresh_encoder_attrs_(*this);                                                           \
  }                                                                                               \
                                                                                                  \
  prep::WrapperName& prep::WrapperName::learn(const pluss::table::DataFrame& frame,       \
                                                      const TrainOptions& options) {              \
    const auto names = resolved_feature_names_(frame, Label ".learn");                           \
    const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),   \
                               Label ".learn.feature_batch_from_dataframe");                      \
    resolve(core_.learn(batch, options), Label ".learn");                                         \
    feature_names_ = names;                                                                       \
    feature_names_pinned_ = true;                                                                 \
    learned_ = true;                                                                              \
    refresh_state_();                                                                             \
    return *this;                                                                                 \
  }                                                                                               \
                                                                                                  \
  pluss::table::DataFrame prep::WrapperName::map(const pluss::table::DataFrame& frame,    \
                                                         const InferOptions& options) const {     \
    if (!learned_) {                                                                              \
      ::pluss::throw_error(std::string(Label) + ".map: call learn() or learn_map() first."); \
    }                                                                                             \
    const auto names = resolved_feature_names_(frame, Label ".map");                             \
    const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),   \
                               Label ".map.feature_batch_from_dataframe");                        \
    const auto mapped = resolve(core_.map(batch, options), Label ".map");                        \
    return merge_mapped_(frame, names, mapped, Label ".map");                                    \
  }                                                                                               \
                                                                                                  \
  pluss::table::DataFrame prep::WrapperName::learn_map(const pluss::table::DataFrame& frame, \
                                                               const TrainOptions& learn_options, \
                                                               const InferOptions& infer_options) { \
    const auto names = resolved_feature_names_(frame, Label ".learn_map");                       \
    const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),   \
                               Label ".learn_map.feature_batch_from_dataframe");                  \
    const auto mapped = resolve(core_.learn_map(batch, learn_options, infer_options), Label ".learn_map"); \
    feature_names_ = names;                                                                       \
    feature_names_pinned_ = true;                                                                 \
    learned_ = true;                                                                              \
    refresh_state_();                                                                             \
    return merge_mapped_(frame, names, mapped, Label ".learn_map");                              \
  }

#define PLUSS_NODEJ_DEFINE_PREP_WRAPPER_WITH_THREAD(WrapperName, Label)                          \
  PLUSS_NODEJ_DEFINE_PREP_WRAPPER(WrapperName, Label)                                             \
                                                                                                  \
  prep::WrapperName& prep::WrapperName::learn(const pluss::table::Thread& feature,        \
                                                      const TrainOptions& options) {              \
    prep_before_thread_fit_(*this, feature);                                                     \
    resolve(core_.learn(feature, options), Label ".learn(Thread)");                              \
    if (feature_names_.empty() && feature.named()) {                                              \
      feature_names_ = {feature.name()};                                                          \
      feature_names_pinned_ = true;                                                               \
    }                                                                                             \
    learned_ = true;                                                                              \
    refresh_state_();                                                                             \
    return *this;                                                                                 \
  }                                                                                               \
                                                                                                  \
  Series prep::WrapperName::map(const pluss::table::Thread& feature,                      \
                                        const InferOptions& options) const {                      \
    if (!learned_) {                                                                              \
      ::pluss::throw_error(std::string(Label) + ".map(Thread): call learn() or learn_map() first."); \
    }                                                                                             \
    return resolve(core_.map(feature, options), Label ".map(Thread)");                           \
  }                                                                                               \
                                                                                                  \
  Series prep::WrapperName::learn_map(const pluss::table::Thread& feature,                \
                                              const TrainOptions& learn_options,                  \
                                              const InferOptions& infer_options) {                \
    prep_before_thread_fit_(*this, feature);                                                     \
    const auto mapped = resolve(core_.learn_map(feature, learn_options, infer_options),          \
                                Label ".learn_map(Thread)");                                      \
    if (feature_names_.empty() && feature.named()) {                                              \
      feature_names_ = {feature.name()};                                                          \
      feature_names_pinned_ = true;                                                               \
    }                                                                                             \
    learned_ = true;                                                                              \
    refresh_state_();                                                                             \
    return mapped;                                                                                \
  }

PLUSS_NODEJ_DEFINE_PREP_WRAPPER(MinMaxScaler, "prep::MinMaxScaler")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(RobustScaler, "prep::RobustScaler")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(NeighborImputer, "prep::NeighborImputer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(IterativeImputer, "prep::IterativeImputer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER_WITH_THREAD(OneHotEncoder, "prep::OneHotEncoder")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER_WITH_THREAD(FrequencyEncoder, "prep::FrequencyEncoder")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER_WITH_THREAD(OrdinalEncoder, "prep::OrdinalEncoder")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER_WITH_THREAD(LabelEncoder, "prep::LabelEncoder")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(VectorNormalizer, "prep::VectorNormalizer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(MaxAbsScaler, "prep::MaxAbsScaler")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(QuantileMap, "prep::QuantileMap")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(PowerMap, "prep::PowerMap")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(ThresholdBinarizer, "prep::ThresholdBinarizer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(PolynomialLift, "prep::PolynomialLift")

#undef PLUSS_NODEJ_DEFINE_PREP_WRAPPER
#undef PLUSS_NODEJ_DEFINE_PREP_WRAPPER_WITH_THREAD

void prep::prep_before_thread_fit_(OneHotEncoder& wrapper, const pluss::table::Thread& feature) {
  wrapper.category_labels_by_key_.clear();
  if (feature.kind() != pluss::table::ThreadKind::Note) return;
  for (std::int64_t i = 0; i < feature.size(); ++i) {
    if (!feature.is_valid(i)) continue;
    const auto token = std::string(feature.as_note().at(i));
    const auto key = prep_key_for_value_(prep_note_hash_value_(token));
    if (wrapper.category_labels_by_key_.find(key) == wrapper.category_labels_by_key_.end()) {
      wrapper.category_labels_by_key_.emplace(key, token);
    }
  }
}

void prep::prep_before_thread_fit_(FrequencyEncoder& wrapper, const pluss::table::Thread& feature) {
  wrapper.category_labels_by_key_.clear();
  if (feature.kind() != pluss::table::ThreadKind::Note) return;
  for (std::int64_t i = 0; i < feature.size(); ++i) {
    if (!feature.is_valid(i)) continue;
    const auto token = std::string(feature.as_note().at(i));
    const auto key = prep_key_for_value_(prep_note_hash_value_(token));
    if (wrapper.category_labels_by_key_.find(key) == wrapper.category_labels_by_key_.end()) {
      wrapper.category_labels_by_key_.emplace(key, token);
    }
  }
}

void prep::prep_before_thread_fit_(OrdinalEncoder& wrapper, const pluss::table::Thread& feature) {
  wrapper.category_labels_by_key_.clear();
  if (feature.kind() != pluss::table::ThreadKind::Note) return;
  for (std::int64_t i = 0; i < feature.size(); ++i) {
    if (!feature.is_valid(i)) continue;
    const auto token = std::string(feature.as_note().at(i));
    const auto key = prep_key_for_value_(prep_note_hash_value_(token));
    if (wrapper.category_labels_by_key_.find(key) == wrapper.category_labels_by_key_.end()) {
      wrapper.category_labels_by_key_.emplace(key, token);
    }
  }
}

void prep::prep_before_thread_fit_(LabelEncoder& wrapper, const pluss::table::Thread& feature) {
  wrapper.category_labels_by_key_.clear();
  if (feature.kind() != pluss::table::ThreadKind::Note) return;
  for (std::int64_t i = 0; i < feature.size(); ++i) {
    if (!feature.is_valid(i)) continue;
    const auto token = std::string(feature.as_note().at(i));
    const auto key = prep_key_for_value_(prep_note_hash_value_(token));
    if (wrapper.category_labels_by_key_.find(key) == wrapper.category_labels_by_key_.end()) {
      wrapper.category_labels_by_key_.emplace(key, token);
    }
  }
}

void prep::prep_refresh_encoder_attrs_(OneHotEncoder& wrapper) {
  wrapper.categories_.clear();
  wrapper.category_keys_.clear();
  wrapper.frequencies_.clear();
  wrapper.mapped_names_.clear();
  if (!wrapper.learned() || wrapper.state_.size() < 5) return;

  const auto key_sizes =
      wrapper.state_[1].to(pluss::pCPU, ScalarType::Int64).reshape({wrapper.state_[1].numel()}).to_vec<std::int64_t>();
  const auto widths =
      wrapper.state_[4].to(pluss::pCPU, ScalarType::Int64).reshape({wrapper.state_[4].numel()}).to_vec<std::int64_t>();
  const auto flat_keys =
      wrapper.state_[2].to(pluss::pCPU, ScalarType::Float64).reshape({wrapper.state_[2].numel()}).to_vec<double>();

  const std::size_t n_features = std::min(key_sizes.size(), widths.size());
  wrapper.categories_.reserve(n_features);
  wrapper.category_keys_.reserve(n_features);

  std::size_t cursor = 0;
  for (std::size_t i = 0; i < n_features; ++i) {
    const std::int64_t key_count = std::max<std::int64_t>(0, key_sizes[i]);
    std::vector<double> key_vals{};
    key_vals.reserve(static_cast<std::size_t>(key_count));
    std::vector<double> cats{};
    cats.reserve(static_cast<std::size_t>(key_count));
    for (std::int64_t j = 0; j < key_count && cursor < flat_keys.size(); ++j, ++cursor) {
      key_vals.push_back(flat_keys[cursor]);
      cats.push_back(flat_keys[cursor]);
    }
    std::vector<std::string> labels{};
    labels.reserve(cats.size());
    for (double cat_key_value : cats) {
      const auto key = prep_key_for_value_(cat_key_value);
      auto it = wrapper.category_labels_by_key_.find(key);
      if (it != wrapper.category_labels_by_key_.end()) {
        labels.push_back(it->second);
      } else {
        labels.push_back(prep_scalar_to_string_(cat_key_value));
      }
    }
    wrapper.category_keys_.push_back(std::move(key_vals));
    wrapper.categories_.push_back(std::move(labels));
  }

  for (std::size_t i = 0; i < n_features; ++i) {
    const std::string base = i < wrapper.feature_names_.size()
                                 ? wrapper.feature_names_[i]
                                 : ("feature_" + std::to_string(i));
    const std::int64_t key_count = std::max<std::int64_t>(0, key_sizes[i]);
    const std::int64_t width = std::max<std::int64_t>(0, widths[i]);
    for (std::int64_t j = 0; j < std::min<std::int64_t>(key_count, width); ++j) {
      const std::string suffix =
          (i < wrapper.categories_.size() && j < static_cast<std::int64_t>(wrapper.categories_[i].size()))
              ? wrapper.categories_[i][static_cast<std::size_t>(j)]
              : std::to_string(j);
      wrapper.mapped_names_.push_back(base + "_" + suffix);
    }
    if (width > key_count) {
      wrapper.mapped_names_.push_back(base + "_unknown");
      for (std::int64_t j = key_count + 1; j < width; ++j) {
        wrapper.mapped_names_.push_back(base + "_" + std::to_string(j));
      }
    }
  }
}

void prep::prep_refresh_encoder_attrs_(FrequencyEncoder& wrapper) {
  wrapper.categories_.clear();
  wrapper.category_keys_.clear();
  wrapper.frequencies_.clear();
  wrapper.mapped_names_.clear();
  if (!wrapper.learned() || wrapper.state_.size() < 5) return;

  const auto key_sizes =
      wrapper.state_[1].to(pluss::pCPU, ScalarType::Int64).reshape({wrapper.state_[1].numel()}).to_vec<std::int64_t>();
  const auto freq_sizes =
      wrapper.state_[3].to(pluss::pCPU, ScalarType::Int64).reshape({wrapper.state_[3].numel()}).to_vec<std::int64_t>();
  const auto flat_keys =
      wrapper.state_[2].to(pluss::pCPU, ScalarType::Float64).reshape({wrapper.state_[2].numel()}).to_vec<double>();
  const auto flat_freqs =
      wrapper.state_[4].to(pluss::pCPU, ScalarType::Float64).reshape({wrapper.state_[4].numel()}).to_vec<double>();

  const std::size_t n_features = std::min(key_sizes.size(), freq_sizes.size());
  wrapper.categories_.reserve(n_features);
  wrapper.category_keys_.reserve(n_features);
  wrapper.frequencies_.reserve(n_features);

  std::size_t key_cursor = 0;
  std::size_t freq_cursor = 0;
  for (std::size_t i = 0; i < n_features; ++i) {
    const std::int64_t key_count = std::max<std::int64_t>(0, key_sizes[i]);
    const std::int64_t freq_count = std::max<std::int64_t>(0, freq_sizes[i]);

    std::vector<double> key_vals{};
    key_vals.reserve(static_cast<std::size_t>(key_count));
    for (std::int64_t j = 0; j < key_count && key_cursor < flat_keys.size(); ++j, ++key_cursor) {
      key_vals.push_back(flat_keys[key_cursor]);
    }

    std::vector<double> freqs{};
    freqs.reserve(static_cast<std::size_t>(freq_count));
    for (std::int64_t j = 0; j < freq_count && freq_cursor < flat_freqs.size(); ++j, ++freq_cursor) {
      freqs.push_back(flat_freqs[freq_cursor]);
    }

    std::vector<std::string> labels{};
    labels.reserve(key_vals.size());
    for (double cat_key_value : key_vals) {
      const auto key = prep_key_for_value_(cat_key_value);
      auto it = wrapper.category_labels_by_key_.find(key);
      if (it != wrapper.category_labels_by_key_.end()) {
        labels.push_back(it->second);
      } else {
        labels.push_back(prep_scalar_to_string_(cat_key_value));
      }
    }

    wrapper.category_keys_.push_back(std::move(key_vals));
    wrapper.categories_.push_back(std::move(labels));
    wrapper.frequencies_.push_back(std::move(freqs));
  }

  for (std::size_t i = 0; i < n_features; ++i) {
    if (i < wrapper.feature_names_.size()) {
      wrapper.mapped_names_.push_back(wrapper.feature_names_[i]);
    } else {
      wrapper.mapped_names_.push_back("feature_" + std::to_string(i));
    }
  }
}

void prep::prep_refresh_encoder_attrs_(OrdinalEncoder& wrapper) {
  wrapper.categories_.clear();
  wrapper.category_keys_.clear();
  wrapper.frequencies_.clear();
  wrapper.mapped_names_.clear();
  if (!wrapper.learned() || wrapper.state_.size() < 3) return;

  const auto key_sizes =
      wrapper.state_[1].to(pluss::pCPU, ScalarType::Int64).reshape({wrapper.state_[1].numel()}).to_vec<std::int64_t>();
  const auto flat_keys =
      wrapper.state_[2].to(pluss::pCPU, ScalarType::Float64).reshape({wrapper.state_[2].numel()}).to_vec<double>();

  const std::size_t n_features = key_sizes.size();
  wrapper.categories_.reserve(n_features);
  wrapper.category_keys_.reserve(n_features);

  std::size_t key_cursor = 0;
  for (std::size_t i = 0; i < n_features; ++i) {
    const std::int64_t key_count = std::max<std::int64_t>(0, key_sizes[i]);
    std::vector<double> key_vals{};
    key_vals.reserve(static_cast<std::size_t>(key_count));
    for (std::int64_t j = 0; j < key_count && key_cursor < flat_keys.size(); ++j, ++key_cursor) {
      key_vals.push_back(flat_keys[key_cursor]);
    }

    std::vector<std::string> labels{};
    labels.reserve(key_vals.size());
    for (double cat_key_value : key_vals) {
      const auto key = prep_key_for_value_(cat_key_value);
      auto it = wrapper.category_labels_by_key_.find(key);
      if (it != wrapper.category_labels_by_key_.end()) {
        labels.push_back(it->second);
      } else {
        labels.push_back(prep_scalar_to_string_(cat_key_value));
      }
    }

    wrapper.category_keys_.push_back(std::move(key_vals));
    wrapper.categories_.push_back(std::move(labels));
    if (i < wrapper.feature_names_.size()) {
      wrapper.mapped_names_.push_back(wrapper.feature_names_[i] + "_ordinal");
    } else {
      wrapper.mapped_names_.push_back("feature_" + std::to_string(i) + "_ordinal");
    }
  }
}

void prep::prep_refresh_encoder_attrs_(LabelEncoder& wrapper) {
  wrapper.categories_.clear();
  wrapper.category_keys_.clear();
  wrapper.frequencies_.clear();
  wrapper.mapped_names_.clear();
  if (!wrapper.learned() || wrapper.state_.size() < 3) return;

  const auto key_sizes =
      wrapper.state_[1].to(pluss::pCPU, ScalarType::Int64).reshape({wrapper.state_[1].numel()}).to_vec<std::int64_t>();
  const auto flat_keys =
      wrapper.state_[2].to(pluss::pCPU, ScalarType::Float64).reshape({wrapper.state_[2].numel()}).to_vec<double>();

  const std::size_t n_features = key_sizes.size();
  wrapper.categories_.reserve(n_features);
  wrapper.category_keys_.reserve(n_features);

  std::size_t key_cursor = 0;
  for (std::size_t i = 0; i < n_features; ++i) {
    const std::int64_t key_count = std::max<std::int64_t>(0, key_sizes[i]);
    std::vector<double> key_vals{};
    key_vals.reserve(static_cast<std::size_t>(key_count));
    for (std::int64_t j = 0; j < key_count && key_cursor < flat_keys.size(); ++j, ++key_cursor) {
      key_vals.push_back(flat_keys[key_cursor]);
    }

    std::vector<std::string> labels{};
    labels.reserve(key_vals.size());
    for (double cat_key_value : key_vals) {
      const auto key = prep_key_for_value_(cat_key_value);
      auto it = wrapper.category_labels_by_key_.find(key);
      if (it != wrapper.category_labels_by_key_.end()) {
        labels.push_back(it->second);
      } else {
        labels.push_back(prep_scalar_to_string_(cat_key_value));
      }
    }

    wrapper.category_keys_.push_back(std::move(key_vals));
    wrapper.categories_.push_back(std::move(labels));
    if (i < wrapper.feature_names_.size()) {
      wrapper.mapped_names_.push_back(wrapper.feature_names_[i] + "_label");
    } else {
      wrapper.mapped_names_.push_back("feature_" + std::to_string(i) + "_label");
    }
  }
}

prep::OneHotEncoderOptions prep::CategoryIndexer::to_one_hot_options_(const CategoryIndexerOptions& options) {
  prep::OneHotEncoderOptions out{};
  out.max_categories = options.max_categories;
  out.overflow = options.overflow;
  out.include_unknown_bucket = options.include_unknown_bucket;
  return out;
}

Series prep::CategoryIndexer::encode_feature_(const OneHotEncoder& encoder,
                                                     const pluss::table::Thread& feature,
                                                     bool one_based,
                                                     const InferOptions& options,
                                                     const char* where) {
  (void)where;
  const Series hot = encoder.map(feature, options);
  if (!hot.defined() || hot.dim() != 2) {
    ::pluss::throw_error(std::string(where) + ": encoded feature matrix must be rank-2.");
  }
  const auto sizes = hot.sizes();
  if (sizes.size() != 2 || sizes[1] <= 0) {
    ::pluss::throw_error(std::string(where) + ": encoded feature matrix must have nonzero width.");
  }
  const std::int64_t rows = sizes[0];
  const std::int64_t width = sizes[1];

  std::vector<double> weights_host(static_cast<std::size_t>(width), 0.0);
  const double base = one_based ? 1.0 : 0.0;
  for (std::int64_t j = 0; j < width; ++j) {
    weights_host[static_cast<std::size_t>(j)] = base + static_cast<double>(j);
  }
  const Series weights = pluss::make_series(
      std::move(weights_host),
      {width, 1},
      Config{}.with_dtype(ScalarType::Float64).with_platform(hot.platform()));
  return hot.matmul(weights).reshape({rows});
}

void prep::CategoryIndexer::refresh_from_encoders_() {
  categories_.clear();
  mapped_names_.clear();
  if (!learned_) return;

  categories_.reserve(encoders_.size());
  mapped_names_.reserve(encoders_.size());
  for (std::size_t i = 0; i < encoders_.size(); ++i) {
    if (!encoders_[i].categories_.empty()) {
      categories_.push_back(encoders_[i].categories_.front());
    } else {
      categories_.emplace_back();
    }
    const std::string base = i < feature_names_.size()
                                 ? feature_names_[i]
                                 : ("feature_" + std::to_string(i));
    mapped_names_.push_back(base + "_index");
  }
}

prep::CategoryIndexer& prep::CategoryIndexer::learn(const pluss::table::Thread& feature,
                                                           const TrainOptions& options) {
  if (!feature.defined()) {
    ::pluss::throw_error("prep::CategoryIndexer.learn(Thread): input feature must be defined.");
  }
  if (feature.named()) {
    feature_names_ = {feature.name()};
    feature_names_pinned_ = true;
  } else if (feature_names_.empty()) {
    feature_names_ = {"feature_0"};
    feature_names_pinned_ = true;
  }

  OneHotEncoder encoder(to_one_hot_options_(options_));
  encoder.learn(feature, options);
  encoders_.clear();
  encoders_.push_back(std::move(encoder));
  learned_ = true;
  refresh_from_encoders_();
  return *this;
}

prep::CategoryIndexer& prep::CategoryIndexer::learn(const pluss::table::DataFrame& frame,
                                                           const TrainOptions& options) {
  if (!feature_names_pinned_ || feature_names_.empty()) {
    ::pluss::throw_error(
        "prep::CategoryIndexer.learn(DataFrame): explicit feature selection is required; call .features({...}).");
  }
  encoders_.clear();
  encoders_.reserve(feature_names_.size());
  for (const auto& name : feature_names_) {
    if (!frame.get(name).has_value()) {
      ::pluss::throw_error("prep::CategoryIndexer.learn(DataFrame): missing feature '" + name + "'.");
    }
    OneHotEncoder encoder(to_one_hot_options_(options_));
    encoder.learn(frame.features(name), options);
    encoders_.push_back(std::move(encoder));
  }
  learned_ = true;
  refresh_from_encoders_();
  return *this;
}

Series prep::CategoryIndexer::map(const pluss::table::Thread& feature, const InferOptions& options) const {
  if (!learned_ || encoders_.empty()) {
    ::pluss::throw_error("prep::CategoryIndexer.map(Thread): call learn() or learn_map() first.");
  }
  if (encoders_.size() != 1) {
    ::pluss::throw_error(
        "prep::CategoryIndexer.map(Thread): encoder was trained for multiple features; map(DataFrame) is required.");
  }
  return encode_feature_(encoders_.front(), feature, options_.one_based, options, "prep::CategoryIndexer.map(Thread)");
}

pluss::table::DataFrame prep::CategoryIndexer::map(const pluss::table::DataFrame& frame,
                                                          const InferOptions& options) const {
  if (!learned_ || encoders_.empty()) {
    ::pluss::throw_error("prep::CategoryIndexer.map(DataFrame): call learn() or learn_map() first.");
  }
  if (encoders_.size() != feature_names_.size()) {
    ::pluss::throw_error("prep::CategoryIndexer.map(DataFrame): internal encoder state is inconsistent.");
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> columns{};
  columns.reserve(feature_names_.size());
  for (std::size_t i = 0; i < feature_names_.size(); ++i) {
    const auto& name = feature_names_[i];
    if (!frame.get(name).has_value()) {
      ::pluss::throw_error("prep::CategoryIndexer.map(DataFrame): missing feature '" + name + "'.");
    }
    const auto& encoder = encoders_[i];
    const Series idx = encode_feature_(
        encoder,
        frame.features(name),
        options_.one_based,
        options,
        "prep::CategoryIndexer.map(DataFrame)");
    const std::string out_name = i < mapped_names_.size()
                                     ? mapped_names_[i]
                                     : (name + "_index");
    columns.emplace_back(out_name, pluss::table::Thread(idx.reshape({frame.ncases()})));
  }
  return pluss::table::DataFrame(std::move(columns), frame.label().values());
}

Series prep::CategoryIndexer::learn_map(const pluss::table::Thread& feature,
                                               const TrainOptions& learn_options,
                                               const InferOptions& infer_options) {
  learn(feature, learn_options);
  return map(feature, infer_options);
}

pluss::table::DataFrame prep::CategoryIndexer::learn_map(const pluss::table::DataFrame& frame,
                                                                const TrainOptions& learn_options,
                                                                const InferOptions& infer_options) {
  learn(frame, learn_options);
  return map(frame, infer_options);
}


} // namespace pluss::ai::lymph
