Thread::Thread(pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl) : impl_(std::move(impl)) {}

Thread::Thread() = default;

namespace {

std::optional<pluss::Note> range_display_labels_from_options_(std::int64_t size,
                                                              const ThreadOptions& options) {
  if (!options.rangelbl) {
    return std::nullopt;
  }
  PLUSS_CHECK(size >= 0, "Thread: range labels size must be non-negative");
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(size));
  for (std::int64_t i = 0; i < size; ++i) {
    labels.push_back(std::to_string(i));
  }
  return make_utf8_note(labels);
}

pluss::msharp::intrusive_ptr<detail::ThreadImpl> build_thread_impl_with_options_(
    Payload payload,
    std::optional<pluss::Series> validity_mask,
    const ThreadOptions& options) {
  const auto size = std::visit([](const auto& value) { return infer_size(value); }, payload);
  return build_impl(std::move(payload),
                    std::move(validity_mask),
                    range_display_labels_from_options_(size, options),
                    std::nullopt,
                    options.rangelbl ? LabelKind::Case : LabelKind::Default);
}

} // namespace

Thread::Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Series& data, const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Thread::Thread(const pluss::Series& data,
               std::optional<pluss::Series> validity_mask,
               const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Thread::Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Note& data, const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Thread::Thread(const pluss::Note& data,
               std::optional<pluss::Series> validity_mask,
               const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Thread::Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Timeline& data, const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Thread::Thread(const pluss::Timeline& data,
               std::optional<pluss::Series> validity_mask,
               const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Thread::Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::TimeIndex& data, const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Thread::Thread(const pluss::TimeIndex& data,
               std::optional<pluss::Series> validity_mask,
               const ThreadOptions& options)
    : impl_(build_thread_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Thread::Thread(const Thread& other) = default;
Thread::Thread(Thread&& other) noexcept = default;
Thread& Thread::operator=(const Thread& other) = default;
Thread& Thread::operator=(Thread&& other) noexcept = default;
Thread::~Thread() = default;

bool Thread::defined() const noexcept { return static_cast<bool>(impl_); }
bool Thread::unique() const noexcept { return impl_.unique(); }
bool Thread::shares_payload_with(const Thread& other) const noexcept {
  return impl_.get() == other.impl_.get();
}

const detail::ThreadImpl& Thread::impl_ref_() const {
  PLUSS_CHECK(defined(), "Thread: undefined handle");
  return *impl_;
}

detail::ThreadImpl& Thread::mut_impl_() {
  PLUSS_CHECK(defined(), "Thread: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

namespace detail {

LabelKind label_kind(const Thread& thread) {
  return thread.impl_ref_().label_kind_;
}

Thread with_label_kind(const Thread& thread, LabelKind kind) {
  auto out = thread;
  return with_label_kind_(out, kind);
}

Thread& with_label_kind_(Thread& thread, LabelKind kind) {
  thread.mut_impl_().label_kind_ = kind;
  return thread;
}

} // namespace detail

std::int64_t Thread::size() const { return impl_ref_().size_; }
std::int64_t Thread::ncases() const { return size(); }
std::pair<std::int64_t, std::int64_t> Thread::shape() const {
  return {size(), 1};
}
ThreadKind Thread::kind() const { return impl_ref_().kind_; }
std::string Thread::dtype() const { return impl_ref_().dtype_; }
ThreadValue Thread::value() const { return ThreadValue(*this); }
bool Thread::is_valid(std::int64_t row) const {
  const auto idx = normalize_index(row, size());
  if (!has_validity_mask()) {
    return true;
  }
  const auto valid = validity_mask()->to_vec<bool>();
  return valid[static_cast<std::size_t>(idx)];
}

bool Thread::has_validity_mask() const { return impl_ref_().validity_mask_.has_value(); }

const std::optional<pluss::Series>& Thread::validity_mask() const {
  return impl_ref_().validity_mask_;
}

bool Thread::has_display_labels() const {
  return impl_ref_().display_labels_.has_value();
}

Thread Thread::display_labels() const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.display_labels_.has_value(), "Thread::display_labels: labels are not set");
  return Thread(*src.display_labels_);
}

Thread Thread::with_display_labels(const Thread& labels) const {
  PLUSS_CHECK(labels.defined(), "Thread::with_display_labels: labels are undefined");
  PLUSS_CHECK(labels.size() == size(), "Thread::with_display_labels: labels length mismatch");

  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t i = 0; i < size(); ++i) {
    values.push_back(thread_scalar_string_at(labels, i));
  }

  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_,
                           src.validity_mask_,
                           make_utf8_note(values),
                           src.display_name_,
                           LabelKind::Custom));
}

Thread& Thread::with_display_labels_(const Thread& labels) {
  PLUSS_CHECK(labels.defined(), "Thread::with_display_labels_: labels are undefined");
  PLUSS_CHECK(labels.size() == size(), "Thread::with_display_labels_: labels length mismatch");

  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t i = 0; i < size(); ++i) {
    values.push_back(thread_scalar_string_at(labels, i));
  }

  auto& w = mut_impl_();
  w.display_labels_ = make_utf8_note(values);
  w.label_kind_ = LabelKind::Custom;
  return *this;
}

Thread Thread::clear_display_labels() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, src.validity_mask_, std::nullopt, src.display_name_));
}

Thread& Thread::clear_display_labels_() {
  mut_impl_().display_labels_.reset();
  mut_impl_().label_kind_ = LabelKind::Default;
  return *this;
}

bool Thread::has_caselbl() const {
  return defined();
}

Thread Thread::caselbl() const {
  if (has_display_labels()) {
    return display_labels();
  }
  return default_thread_cases_(size());
}

Thread Thread::caselbl(const Thread& labels) const {
  return with_display_labels(labels);
}

Thread& Thread::caselbl_(const Thread& labels) {
  return with_display_labels_(labels);
}

Thread Thread::rangelbl() const {
  auto out = with_display_labels(default_thread_cases_(size()));
  return detail::with_label_kind(out, LabelKind::Case);
}

Thread& Thread::rangelbl_() {
  with_display_labels_(default_thread_cases_(size()));
  detail::with_label_kind_(*this, LabelKind::Case);
  return *this;
}

Thread Thread::clear_caselbl() const {
  return clear_display_labels();
}

Thread& Thread::clear_caselbl_() {
  return clear_display_labels_();
}

Thread Thread::align(const Thread& target_labels, const AlignOptions& options) const {
  return align_thread_to_label_values_(*this, target_labels, options, "Thread::align");
}

bool Thread::has_display_name() const {
  return impl_ref_().display_name_.has_value();
}

std::string Thread::display_name() const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.display_name_.has_value(), "Thread::display_name: name is not set");
  return *src.display_name_;
}

Thread Thread::with_display_name(std::string name) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!name.empty(), "Thread::with_display_name: name must not be empty");
  return Thread(build_impl(src.payload_,
                           src.validity_mask_,
                           src.display_labels_,
                           std::move(name),
                           src.label_kind_));
}

Thread Thread::clear_display_name() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_,
                           src.validity_mask_,
                           src.display_labels_,
                           std::nullopt,
                           src.label_kind_));
}

Thread Thread::with_validity_mask(const pluss::Series& validity_mask) const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_,
                           validity_mask,
                           src.display_labels_,
                           src.display_name_,
                           src.label_kind_));
}

Thread& Thread::with_validity_mask_(const pluss::Series& validity_mask) {
  auto& w = mut_impl_();
  validate_validity_mask(validity_mask, w.size_);
  w.validity_mask_ = validity_mask;
  return *this;
}

Thread Thread::clear_validity_mask() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_,
                           std::nullopt,
                           src.display_labels_,
                           src.display_name_,
                           src.label_kind_));
}

Thread& Thread::clear_validity_mask_() {
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  return *this;
}

Thread Thread::isnull() const {
  auto out = Thread(thread_non_missing_mask(*this).logical_not());
  return detail::has_display_labels(*this)
             ? detail::with_display_labels(out, detail::display_labels(*this))
             : out;
}

Thread Thread::notnull() const {
  auto out = Thread(thread_non_missing_mask(*this));
  return detail::has_display_labels(*this)
             ? detail::with_display_labels(out, detail::display_labels(*this))
             : out;
}

Thread Thread::isnan() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isnan is only valid for Series payload");
  return Thread(as_series().isnan());
}

Thread Thread::isfinite() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isfinite is only valid for Series payload");
  auto finite = as_series().isfinite();
  if (has_validity_mask()) {
    finite = finite.logical_and(notnull().as_series());
  }
  return Thread(finite);
}

Thread Thread::fillna(double value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::fillna(double) is only valid for Series payload");
  const auto missing = notnull().as_series().logical_not();
  if (!has_validity_mask() && !series_uses_nan_missing(as_series())) {
    return *this;
  }
  auto out = as_series().where(missing, value);
  return Thread(out, std::nullopt);
}

Thread Thread::fillna(std::string_view value) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::fillna(string) is only valid for Note payload");
  if (!has_validity_mask()) {
    return *this;
  }
  const auto valid = validity_mask()->to_vec<bool>();
  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t i = 0; i < size(); ++i) {
    const auto idx = static_cast<std::size_t>(i);
    if (valid[idx]) {
      values.emplace_back(as_note().at(i));
    } else {
      values.emplace_back(value);
    }
  }
  return Thread(rebuild_note_like(as_note(), values), std::nullopt);
}

Thread Thread::fillna(const pluss::Timeline& value) const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex,
              "Thread::fillna(Timeline) is only valid for Timeline/TimeIndex payload");
  PLUSS_CHECK(value.defined(), "Thread::fillna(Timeline): replacement value is undefined");
  PLUSS_CHECK(value.numel() == 1, "Thread::fillna(Timeline): replacement value must be scalar (numel == 1)");
  if (!has_validity_mask()) {
    return *this;
  }

  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().where(*validity_mask(), value), std::nullopt);
  }

  auto filled = Thread(as_timeindex().timeline(), validity_mask()).fillna(value);
  return Thread(filled.as_timeline().to_timeindex(false), std::nullopt);
}

Thread Thread::fillna(const pluss::TimeIndex& value) const {
  PLUSS_CHECK(kind() == ThreadKind::TimeIndex, "Thread::fillna(TimeIndex) is only valid for TimeIndex payload");
  PLUSS_CHECK(value.timeline().defined(), "Thread::fillna(TimeIndex): replacement value is undefined");
  PLUSS_CHECK(value.size() == 1, "Thread::fillna(TimeIndex): replacement value must be scalar (size == 1)");
  return fillna(value.timeline());
}

namespace {

pluss::Series thread_put_mask_(const Thread& bool_mask,
                               std::int64_t expected_size,
                               std::string_view where) {
  PLUSS_CHECK(bool_mask.defined(), std::string(where) + ": mask is undefined");
  PLUSS_CHECK(bool_mask.kind() == ThreadKind::Series, std::string(where) + ": mask must be a Series thread");
  const auto& mask = bool_mask.as_series();
  (void)bool_vec_from_series(mask, expected_size, where);
  return mask;
}

void validate_thread_put_value_(const Thread& base,
                                const Thread& value,
                                std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": replacement value is undefined");
  PLUSS_CHECK(value.kind() == base.kind(),
              std::string(where) + ": replacement kind must match target thread kind");
  PLUSS_CHECK(value.size() == 1 || value.size() == base.size(),
              std::string(where) + ": replacement value must be scalar-like or match target length");
}

std::optional<pluss::Series> nullify_matches_mask(const Thread& t,
                                                  const pluss::Series& matches,
                                                  bool& changed);

} // namespace

Thread Thread::put(const Thread& bool_mask, const Thread& value) const {
  const auto aligned_mask = align_thread_to_cases_(bool_mask, *this, "Thread::put(mask)");
  const auto aligned_value = align_thread_to_cases_(value, *this, "Thread::put(value)", true);
  const auto mask = thread_put_mask_(aligned_mask, size(), "Thread::put");
  validate_thread_put_value_(*this, aligned_value, "Thread::put");

  if (kind() == ThreadKind::Note) {
    const auto hit = mask.to_vec<bool>();
    auto out_valid = notnull().as_series().to_vec<bool>();
    std::vector<std::string> out_values;
    out_values.reserve(static_cast<std::size_t>(size()));
    const bool scalar_value = aligned_value.size() == 1;
    for (std::int64_t i = 0; i < size(); ++i) {
      const auto idx = static_cast<std::size_t>(i);
      if (hit[idx]) {
        const auto rhs_row = scalar_value ? static_cast<std::int64_t>(0) : i;
        out_values.emplace_back(aligned_value.as_note().at(rhs_row));
        out_valid[idx] = aligned_value.is_valid(rhs_row);
      } else {
        out_values.emplace_back(as_note().at(i));
      }
    }
    return Thread(rebuild_note_like(as_note(), out_values), optional_mask_from_valid(out_valid));
  }

  return select({aligned_mask}, {aligned_value}, *this);
}

Thread Thread::put(const Thread& bool_mask, double value) const {
  if (kind() == ThreadKind::Series) {
    const auto& s = as_series();
    return put(bool_mask, Thread(pluss::Series::full({1}, value, s.scalar_type(), s.platform())));
  }
  return put(bool_mask, Thread(pluss::make_series(std::vector<double>{value}, {1})));
}

Thread Thread::put(const Thread& bool_mask, std::string_view value) const {
  return put(bool_mask, Thread(pluss::make_note(std::vector<std::string>{std::string(value)})));
}

Thread Thread::put(const Thread& bool_mask, pluss::null_t) const {
  const auto aligned_mask = align_thread_to_cases_(bool_mask, *this, "Thread::put(mask)");
  const auto mask = thread_put_mask_(aligned_mask, size(), "Thread::put");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, mask, changed);
  if (!changed) {
    return *this;
  }

  switch (kind()) {
    case ThreadKind::Series:
      return Thread(as_series(), new_mask);
    case ThreadKind::Note:
      return Thread(as_note(), new_mask);
    case ThreadKind::Timeline:
      return Thread(as_timeline(), new_mask);
    case ThreadKind::TimeIndex:
      return Thread(as_timeindex(), new_mask);
    default:
      PLUSS_FAIL("Thread::put(null): unsupported payload kind");
  }
}

Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                      pluss::msharp::RangeView::Of<Thread> choices,
                      const Thread& default_value) {
  PLUSS_CHECK(!conditions.empty(), "Thread::select: at least one condition is required");
  PLUSS_CHECK(conditions.size() == choices.size(),
              "Thread::select: condition/choice count mismatch");
  PLUSS_CHECK(default_value.defined(), "Thread::select: default value is undefined");

  const auto out_kind = choices[0].kind();
  PLUSS_CHECK(default_value.kind() == out_kind,
              "Thread::select: default value kind must match choice kind");

  std::vector<pluss::Series> cond_masks;
  cond_masks.reserve(conditions.size());
  for (const auto& cond : conditions) {
    PLUSS_CHECK(cond.defined(), "Thread::select: condition is undefined");
    PLUSS_CHECK(cond.kind() == ThreadKind::Series,
                "Thread::select: conditions must be Series-backed boolean masks");
    PLUSS_CHECK(cond.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
                "Thread::select: conditions must use Bool dtype");
    cond_masks.push_back(cond.as_series());
  }
  const auto output_size = cond_masks.front().numel();
  for (const auto& mask : cond_masks) {
    PLUSS_CHECK(mask.numel() == output_size,
                "Thread::select: all conditions must have the same length");
  }

  auto validity_series = [](const Thread& t) -> pluss::Series {
    if (t.has_validity_mask()) {
      return t.notnull().as_series();
    }
    return pluss::Series::ones({1}, pluss::codem::ScalarType::Bool);
  };

  std::vector<pluss::Series> choice_validity;
  choice_validity.reserve(choices.size());
  for (const auto& choice : choices) {
    PLUSS_CHECK(choice.defined(), "Thread::select: choice is undefined");
    PLUSS_CHECK(choice.kind() == out_kind,
                "Thread::select: all choices must use the same payload kind");
    PLUSS_CHECK(choice.size() == 1 || choice.size() == output_size,
                "Thread::select: each choice must be scalar-like or match condition length");
    choice_validity.push_back(validity_series(choice));
  }
  PLUSS_CHECK(default_value.size() == 1 || default_value.size() == output_size,
              "Thread::select: default value must be scalar-like or match condition length");

  auto expand_validity = [](const pluss::Series& valid, std::int64_t output_size) {
    if (valid.numel() == output_size) {
      return valid;
    }
    PLUSS_CHECK(valid.numel() == 1,
                "Thread::select: validity mask must match output size or be scalar-like");
    const bool keep = valid.to_vec<bool>()[0];
    return pluss::Series::full(
        {output_size},
        keep ? 1.0 : 0.0,
        pluss::codem::ScalarType::Bool,
        valid.platform());
  };

  switch (out_kind) {
    case ThreadKind::Series: {
      std::vector<pluss::Series> payloads;
      payloads.reserve(choices.size());
      for (const auto& choice : choices) {
        payloads.push_back(choice.as_series());
      }
      auto out = pluss::select(
          pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
          pluss::msharp::RangeView::Of<pluss::Series>(payloads),
          default_value.as_series());

      std::vector<pluss::Series> expanded_validity;
      expanded_validity.reserve(choice_validity.size());
      for (const auto& valid : choice_validity) {
        expanded_validity.push_back(expand_validity(valid, out.numel()));
      }
      const auto out_valid = optional_mask_from_valid(
          pluss::select(
              pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
              pluss::msharp::RangeView::Of<pluss::Series>(expanded_validity),
              expand_validity(validity_series(default_value), out.numel()))
              .to_vec<bool>());
      return Thread(out, std::move(out_valid));
    }

    case ThreadKind::Timeline: {
      auto out = default_value.as_timeline();
      for (std::size_t i = conditions.size(); i > 0; --i) {
        out = choices[i - 1].as_timeline().where(cond_masks[i - 1], out);
      }

      std::vector<pluss::Series> expanded_validity;
      expanded_validity.reserve(choice_validity.size());
      for (const auto& valid : choice_validity) {
        expanded_validity.push_back(expand_validity(valid, out.numel()));
      }
      const auto out_valid = optional_mask_from_valid(
          pluss::select(
              pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
              pluss::msharp::RangeView::Of<pluss::Series>(expanded_validity),
              expand_validity(validity_series(default_value), out.numel()))
              .to_vec<bool>());
      return Thread(out, std::move(out_valid));
    }

    case ThreadKind::TimeIndex: {
      auto out = default_value.as_timeindex().timeline();
      for (std::size_t i = conditions.size(); i > 0; --i) {
        out = choices[i - 1].as_timeindex().timeline().where(cond_masks[i - 1], out);
      }

      std::vector<pluss::Series> expanded_validity;
      expanded_validity.reserve(choice_validity.size());
      for (const auto& valid : choice_validity) {
        expanded_validity.push_back(expand_validity(valid, out.numel()));
      }
      const auto out_valid = optional_mask_from_valid(
          pluss::select(
              pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
              pluss::msharp::RangeView::Of<pluss::Series>(expanded_validity),
              expand_validity(validity_series(default_value), out.numel()))
              .to_vec<bool>());
      return Thread(out.to_timeindex(false), std::move(out_valid));
    }

    case ThreadKind::Note: {
      std::vector<std::string> out_values;
      out_values.reserve(static_cast<std::size_t>(output_size));
      auto out_valid = expand_validity(validity_series(default_value), output_size).to_vec<bool>();
      const bool default_scalar = default_value.size() == 1;
      for (std::int64_t i = 0; i < output_size; ++i) {
        out_values.emplace_back(default_value.as_note().at(default_scalar ? 0 : i));
      }

      std::vector<std::vector<bool>> cond_hits;
      cond_hits.reserve(cond_masks.size());
      for (const auto& cond_mask : cond_masks) {
        cond_hits.push_back(cond_mask.to_vec<bool>());
      }

      for (std::size_t cond_idx = conditions.size(); cond_idx > 0; --cond_idx) {
        const auto& choice = choices[cond_idx - 1];
        const bool scalar_choice = choice.size() == 1;
        const auto choice_valid = expand_validity(choice_validity[cond_idx - 1], output_size).to_vec<bool>();
        const auto& hits = cond_hits[cond_idx - 1];
        for (std::int64_t row = 0; row < output_size; ++row) {
          const auto row_idx = static_cast<std::size_t>(row);
          if (!hits[row_idx]) {
            continue;
          }
          const auto rhs_row = scalar_choice ? static_cast<std::int64_t>(0) : row;
          out_values[row_idx] = std::string(choice.as_note().at(rhs_row));
          out_valid[row_idx] = choice_valid[row_idx];
        }
      }

      return Thread(rebuild_note_like(default_value.as_note(), out_values),
                    optional_mask_from_valid(out_valid));
    }
    default:
      PLUSS_FAIL("Thread::select: unsupported payload kind");
  }
}

Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                      pluss::msharp::RangeView::Of<Thread> choices,
                      double default_value) {
  PLUSS_CHECK(!choices.empty(), "Thread::select(double): at least one choice is required");
  PLUSS_CHECK(choices[0].kind() == ThreadKind::Series,
              "Thread::select(double): choices must be Series-backed");
  return select(
      conditions,
      choices,
      Thread(pluss::Series::full(
          {1},
          default_value,
          choices[0].as_series().scalar_type(),
          choices[0].as_series().platform())));
}

Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                      pluss::msharp::RangeView::Of<Thread> choices,
                      std::string_view default_value) {
  return select(
      conditions,
      choices,
      Thread(pluss::make_note(std::vector<std::string>{std::string(default_value)})));
}

Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                      pluss::msharp::RangeView::Of<Thread> choices,
                      const pluss::Timeline& default_value) {
  PLUSS_CHECK(default_value.defined(), "Thread::select(Timeline): default value is undefined");
  PLUSS_CHECK(default_value.numel() == 1, "Thread::select(Timeline): default value must be scalar-like");
  return select(
      conditions,
      choices,
      Thread(pluss::make_timeline(
          std::vector<std::string>{std::string(default_value.to_note_iso().at(0))})));
}

Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                      pluss::msharp::RangeView::Of<Thread> choices,
                      const pluss::TimeIndex& default_value) {
  PLUSS_CHECK(default_value.timeline().defined(), "Thread::select(TimeIndex): default value is undefined");
  PLUSS_CHECK(default_value.size() == 1, "Thread::select(TimeIndex): default value must be scalar-like");
  return select(
      conditions,
      choices,
      Thread(pluss::make_timeindex(
          std::vector<std::string>{std::string(default_value.to_note_iso().at(0))})));
}

Thread Thread::select(pluss::msharp::RangeView::Of<Thread> conditions,
                      pluss::msharp::RangeView::Of<Thread> choices,
                      pluss::null_t) {
  PLUSS_CHECK(!choices.empty(), "Thread::select(null): at least one choice is required");
  const auto& seed = choices[0];
  if (seed.kind() == ThreadKind::Note) {
    std::vector<std::string> empty_values(
        static_cast<std::size_t>(seed.size()), std::string{});
    auto empty = Thread(pluss::make_note(empty_values));
    return select(
        conditions,
        choices,
        empty.with_validity_mask(
            pluss::Series::zeros(
                {empty.size()}, pluss::codem::ScalarType::Bool)));
  }
  return select(
      conditions,
      choices,
      seed.with_validity_mask(pluss::Series::zeros({seed.size()}, pluss::codem::ScalarType::Bool)));
}

Thread Thread::abs() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::abs is only valid for Series payload");
  return Thread(as_series().abs(), validity_mask());
}

Thread Thread::clip(double min_value, double max_value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::clip is only valid for Series payload");
  return Thread(as_series().clamp(min_value, max_value), validity_mask());
}

Thread Thread::round(std::int64_t decimals) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::round is only valid for Series payload");
  auto s = as_series();
  if (decimals == 0) {
    return Thread(s.round(), validity_mask());
  }
  const auto scale = std::pow(10.0, static_cast<double>(decimals));
  return Thread(s.mul(scale).round().div(scale), validity_mask());
}

Thread Thread::log() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::log is only valid for Series payload");
  auto s = as_series();
  if (s.scalar_type() != pluss::codem::ScalarType::Float16
      && s.scalar_type() != pluss::codem::ScalarType::BFloat16
      && s.scalar_type() != pluss::codem::ScalarType::Float32
      && s.scalar_type() != pluss::codem::ScalarType::Float64) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  return Thread(s.log(), validity_mask());
}

Thread Thread::exp() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::exp is only valid for Series payload");
  auto s = as_series();
  if (s.scalar_type() != pluss::codem::ScalarType::Float16
      && s.scalar_type() != pluss::codem::ScalarType::BFloat16
      && s.scalar_type() != pluss::codem::ScalarType::Float32
      && s.scalar_type() != pluss::codem::ScalarType::Float64) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  return Thread(s.exp(), validity_mask());
}

Thread& Thread::fillna_(double value) {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::fillna_(double) is only valid for Series payload");
  if (!has_validity_mask()) {
    return *this;
  }
  auto& w = mut_impl_();
  auto* s = std::get_if<pluss::Series>(&w.payload_);
  PLUSS_CHECK(s != nullptr, "Thread::fillna_: internal payload mismatch");
  *s = s->where(w.validity_mask_->logical_not(), value);
  w.validity_mask_.reset();
  return *this;
}

Thread& Thread::fillna_(std::string_view value) {
  *this = fillna(value);
  return *this;
}

Thread& Thread::fillna_(const pluss::Timeline& value) {
  *this = fillna(value);
  return *this;
}

Thread& Thread::fillna_(const pluss::TimeIndex& value) {
  *this = fillna(value);
  return *this;
}

ThreadView Thread::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = slice(0, clipped);
  return ThreadView(std::move(out), render, "head");
}

ThreadView Thread::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = slice(size() - clipped, size());
  return ThreadView(std::move(out), render, "tail");
}

ThreadView Thread::view(std::int64_t start, std::int64_t stop, bool render) const {
  auto out = slice(start, stop);
  return ThreadView(std::move(out), render, "view");
}

ThreadView Thread::top(std::int64_t n, bool render) const {
  PLUSS_CHECK(n >= 0, "Thread::top: n must be >= 0");
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = sort_values(false, true).slice(0, clipped);
  return ThreadView(std::move(out), render, "top");
}

ThreadView Thread::bottom(std::int64_t n, bool render) const {
  PLUSS_CHECK(n >= 0, "Thread::bottom: n must be >= 0");
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = sort_values(true, true).slice(0, clipped);
  return ThreadView(std::move(out), render, "bottom");
}

namespace {

std::optional<pluss::Series> append_validity_mask_(const Thread& left,
                                                   const Thread& right,
                                                   pluss::codem::Platform platform) {
  if (!detail::has_validity_mask(left) && !detail::has_validity_mask(right)) {
    return std::nullopt;
  }
  auto left_mask = detail::has_validity_mask(left)
      ? *detail::validity_mask(left)
      : pluss::Series::ones({left.size()}, pluss::codem::ScalarType::Bool, platform);
  auto right_mask = detail::has_validity_mask(right)
      ? *detail::validity_mask(right)
      : pluss::Series::ones({right.size()}, pluss::codem::ScalarType::Bool, platform);
  if (left_mask.platform() != platform) {
    left_mask = left_mask.to(platform);
  }
  if (right_mask.platform() != platform) {
    right_mask = right_mask.to(platform);
  }
  std::vector<pluss::Series> tail{right_mask};
  return left_mask.unite(tail, 0);
}

Thread append_payload_(const Thread& left, const Thread& right, std::string_view where) {
  PLUSS_CHECK(left.kind() == right.kind(),
              std::string(where) + ": payload kinds must match");
  switch (left.kind()) {
    case ThreadKind::Series: {
      PLUSS_CHECK(left.as_series().scalar_type() == right.as_series().scalar_type(),
                  std::string(where) + ": Series dtypes must match");
      PLUSS_CHECK(left.as_series().platform() == right.as_series().platform(),
                  std::string(where) + ": Series platforms must match");
      std::vector<pluss::Series> tail{right.as_series()};
      auto values = left.as_series().unite(tail, 0);
      return Thread(values, append_validity_mask_(left, right, values.platform()));
    }
    case ThreadKind::Note: {
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(left.size() + right.size()));
      for (const auto value : left.as_note().vector()) {
        values.emplace_back(value);
      }
      for (const auto value : right.as_note().vector()) {
        values.emplace_back(value);
      }
      return Thread(pluss::make_note(values),
                    append_validity_mask_(left, right, pluss::codem::Platform::CPU));
    }
    case ThreadKind::Timeline: {
      PLUSS_CHECK(thread_payload_platform_(left) == thread_payload_platform_(right),
                  std::string(where) + ": Timeline platforms must match");
      auto values = left.as_timeline().concat(right.as_timeline());
      return Thread(values, append_validity_mask_(left, right, thread_payload_platform_(left)));
    }
    case ThreadKind::TimeIndex: {
      PLUSS_CHECK(thread_payload_platform_(left) == thread_payload_platform_(right),
                  std::string(where) + ": TimeIndex platforms must match");
      auto values = left.as_timeindex()
                        .timeline()
                        .concat(right.as_timeindex().timeline())
                        .to_timeindex(false);
      return Thread(values, append_validity_mask_(left, right, thread_payload_platform_(left)));
    }
  }
  PLUSS_FAIL(std::string(where) + ": unsupported Thread kind");
}

std::vector<std::int64_t> thread_sample_indices_(std::int64_t total,
                                                 std::int64_t draw,
                                                 bool replace,
                                                 std::mt19937_64& rng) {
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(draw));
  if (draw == 0 || total == 0) {
    return idx;
  }
  if (replace) {
    std::uniform_int_distribution<std::int64_t> dist(0, total - 1);
    for (std::int64_t i = 0; i < draw; ++i) {
      idx.push_back(dist(rng));
    }
    return idx;
  }

  idx.resize(static_cast<std::size_t>(total));
  std::iota(idx.begin(), idx.end(), std::int64_t{0});
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(draw));
  return idx;
}

} // namespace

Thread Thread::append(const Thread& other) const {
  auto out = append_payload_(*this, other, "Thread::append");
  PLUSS_CHECK(has_caselbl() == other.has_caselbl(),
              "Thread::append: both Threads must either have case labels or both be unlabeled");
  if (has_caselbl()) {
    out = out.caselbl(append_payload_(caselbl(), other.caselbl(), "Thread::append(caselbl)"));
  }
  return out;
}

Thread Thread::sample(std::int64_t n) const {
  return sample(n, std::random_device{}(), false);
}

Thread Thread::sample(std::int64_t n, std::uint64_t random_state, bool replace) const {
  PLUSS_CHECK(n >= 0, "Thread::sample: n must be >= 0");
  const auto total = size();
  const auto draw = replace ? n : std::min<std::int64_t>(n, total);
  std::mt19937_64 rng(random_state);
  return take(thread_sample_indices_(total, draw, replace, rng));
}

Thread Thread::sample_fraction_(double frac,
                                std::optional<std::uint64_t> random_state,
                                bool replace) const {
  PLUSS_CHECK(std::isfinite(frac), "Thread::sample: frac must be finite");
  PLUSS_CHECK(frac >= 0.0, "Thread::sample: frac must be >= 0");
  PLUSS_CHECK(replace || frac <= 1.0, "Thread::sample: frac > 1 requires replace = true");
  const auto draw = static_cast<std::int64_t>(
      std::llround(frac * static_cast<double>(size())));
  return sample(draw, random_state.value_or(std::random_device{}()), replace);
}

Thread Thread::shuffle(std::optional<std::uint64_t> random_state) const {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(size()));
  std::iota(idx.begin(), idx.end(), std::int64_t{0});
  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));
  std::shuffle(idx.begin(), idx.end(), rng);
  return take(idx);
}

Thread Thread::shift(std::int64_t periods) const {
  const auto n = size();
  if (n <= 0 || periods == 0) {
    return *this;
  }

  std::vector<std::int64_t> out_rows(static_cast<std::size_t>(n), 0);
  std::iota(out_rows.begin(), out_rows.end(), static_cast<std::int64_t>(0));

  std::vector<std::int64_t> indices(static_cast<std::size_t>(n), 0);
  std::transform(out_rows.begin(), out_rows.end(), indices.begin(), [&](std::int64_t out_row) {
    const auto src_row = out_row - periods;
    return (src_row >= 0 && src_row < n) ? src_row : static_cast<std::int64_t>(0);
  });

  std::vector<bool> out_valid(static_cast<std::size_t>(n), false);
  std::optional<std::vector<bool>> src_valid = std::nullopt;
  if (has_validity_mask()) {
    src_valid = validity_mask()->to_vec<bool>();
  }

  std::transform(out_rows.begin(), out_rows.end(), out_valid.begin(), [&](std::int64_t out_row) {
    const auto src_row = out_row - periods;
    if (src_row < 0 || src_row >= n) {
      return false;
    }
    return src_valid.has_value() ? (*src_valid)[static_cast<std::size_t>(src_row)] : true;
  });

  auto shifted = take(indices);
  if (std::all_of(out_valid.begin(), out_valid.end(), [](bool v) { return v; })) {
    return shifted.clear_validity_mask();
  }

  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  std::copy(out_valid.begin(), out_valid.end(), p);
  return shifted.with_validity_mask(mask);
}

Thread Thread::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, size());
  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    indices.push_back(i);
  }
  return take(indices);
}

Thread Thread::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  Payload out_payload = std::visit([&](const auto& payload) -> Payload {
    using T = std::decay_t<decltype(payload)>;
    if constexpr (std::is_same_v<T, pluss::Series>) {
      std::vector<std::int64_t> idx_norm;
      idx_norm.reserve(indices.size());
      for (const auto raw_idx : indices) {
        idx_norm.push_back(normalize_index(raw_idx, src.size_));
      }
      auto idx_series = int64_index_series(idx_norm);
      if (idx_series.platform() != payload.platform()) {
        idx_series = idx_series.to(payload.platform());
      }
      return payload.take(idx_series);
    } else if constexpr (std::is_same_v<T, pluss::Note>) {
      const auto values = note_take_strings(payload, indices, src.size_);
      if (static_cast<const pluss::coden::NoteBase&>(payload).is_bytes()) {
        std::vector<std::string_view> views;
        views.reserve(values.size());
        for (const auto& v : values) views.emplace_back(v);
        const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
        return pluss::coden::ops_internal::note_from_bytes(
            std::span<const std::string_view>(views.data(), views.size()),
            std::span<const std::int64_t>(shape.data(), shape.size()));
      }
      std::vector<std::string_view> views;
      views.reserve(values.size());
      for (const auto& v : values) views.emplace_back(v);
      const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
      return pluss::coden::ops_internal::note_from_utf8(
          std::span<const std::string_view>(views.data(), views.size()),
          std::span<const std::int64_t>(shape.data(), shape.size()));
    } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
      return timeline_take(payload, indices, src.size_);
    } else {
      auto t = timeline_take(payload.timeline(), indices, src.size_);
      return t.to_timeindex(false);
    }
  }, src.payload_);

  std::optional<pluss::Series> out_mask = std::nullopt;
  if (src.validity_mask_.has_value()) {
    std::vector<std::int64_t> idx_norm;
    idx_norm.reserve(indices.size());
    for (const auto raw_idx : indices) {
      idx_norm.push_back(normalize_index(raw_idx, src.size_));
    }
    auto idx_series = int64_index_series(idx_norm);
    if (idx_series.platform() != src.validity_mask_->platform()) {
      idx_series = idx_series.to(src.validity_mask_->platform());
    }
    out_mask = src.validity_mask_->take(idx_series);
  }

  std::optional<pluss::Note> out_display_labels = std::nullopt;
  if (src.display_labels_.has_value()) {
    out_display_labels = make_utf8_note(note_take_strings(*src.display_labels_, indices, src.size_));
  }

  return Thread(build_impl(std::move(out_payload),
                           std::move(out_mask),
                           std::move(out_display_labels),
                           src.display_name_,
                           src.label_kind_));
}

Thread Thread::filter(const pluss::Series& bool_mask) const {
  const auto indices = true_indices_from_mask(bool_mask, size());
  return take(indices);
}

Thread Thread::dropnull() const {
  if (!has_validity_mask()) {
    return *this;
  }
  const auto valid = validity_mask()->to_vec<bool>();
  std::vector<std::int64_t> indices;
  indices.reserve(valid.size());
  for (std::size_t i = 0; i < valid.size(); ++i) {
    if (valid[i]) {
      indices.push_back(static_cast<std::int64_t>(i));
    }
  }
  auto out = take(indices);
  if (out.has_validity_mask()) {
    return out.clear_validity_mask();
  }
  return out;
}

Thread Thread::sort_values(bool ascending, bool nulls_last) const {
  if (size() <= 1) {
    return *this;
  }

  if (kind() == ThreadKind::Series && is_numeric_series_type(as_series())) {
    const auto values = as_series();

    if (!has_validity_mask()) {
      const auto order = values.argsort(/*dim=*/0, /*descending=*/!ascending, /*stable=*/true)
                             .to_vec<std::int64_t>();
      return take(order);
    }

    const auto valid = validity_mask()->to_vec<bool>();
    std::vector<std::int64_t> valid_idx;
    std::vector<std::int64_t> null_idx;
    valid_idx.reserve(static_cast<std::size_t>(size()));
    null_idx.reserve(static_cast<std::size_t>(size()));
    for (std::int64_t i = 0; i < size(); ++i) {
      if (valid[static_cast<std::size_t>(i)]) {
        valid_idx.push_back(i);
      } else {
        null_idx.push_back(i);
      }
    }

    std::vector<std::int64_t> sorted_valid_global_idx;
    sorted_valid_global_idx.reserve(valid_idx.size());
    if (!valid_idx.empty()) {
      const auto valid_order_local = values.take(int64_index_series(valid_idx))
                                         .argsort(/*dim=*/0, /*descending=*/!ascending, /*stable=*/true)
                                         .to_vec<std::int64_t>();
      for (const auto local_idx : valid_order_local) {
        sorted_valid_global_idx.push_back(valid_idx[static_cast<std::size_t>(local_idx)]);
      }
    }

    std::vector<std::int64_t> order;
    order.reserve(static_cast<std::size_t>(size()));
    if (nulls_last) {
      order.insert(order.end(), sorted_valid_global_idx.begin(), sorted_valid_global_idx.end());
      order.insert(order.end(), null_idx.begin(), null_idx.end());
    } else {
      order.insert(order.end(), null_idx.begin(), null_idx.end());
      order.insert(order.end(), sorted_valid_global_idx.begin(), sorted_valid_global_idx.end());
    }

    return take(order);
  }

  if (kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex) {
    const auto ticks = (kind() == ThreadKind::Timeline)
        ? as_timeline().to_epoch_nanos_vector()
        : as_timeindex().timeline().to_epoch_nanos_vector();
    std::optional<std::vector<bool>> valid = std::nullopt;
    if (has_validity_mask()) {
      valid = validity_mask()->to_vec<bool>();
    }

    std::vector<std::int64_t> indices(static_cast<std::size_t>(size()));
    std::iota(indices.begin(), indices.end(), static_cast<std::int64_t>(0));

    std::stable_sort(indices.begin(), indices.end(), [&](std::int64_t lhs, std::int64_t rhs) {
      if (valid.has_value()) {
        const bool lv = (*valid)[static_cast<std::size_t>(lhs)];
        const bool rv = (*valid)[static_cast<std::size_t>(rhs)];
        if (lv != rv) {
          return nulls_last ? lv : !lv;
        }
      }

      const auto lt = ticks[static_cast<std::size_t>(lhs)];
      const auto rt = ticks[static_cast<std::size_t>(rhs)];
      if (lt == rt) {
        return false;
      }
      return ascending ? (lt < rt) : (lt > rt);
    });

    return take(indices);
  }

  if (kind() == ThreadKind::Note || (kind() == ThreadKind::Series && !is_numeric_series_type(as_series()))) {
    std::vector<std::string> keys;
    keys.reserve(static_cast<std::size_t>(size()));
    if (kind() == ThreadKind::Note) {
      const auto& note = as_note();
      for (std::int64_t i = 0; i < size(); ++i) {
        keys.emplace_back(note.at(i));
      }
    } else {
      for (std::int64_t i = 0; i < size(); ++i) {
        keys.emplace_back(thread_scalar_string_at(*this, i));
      }
    }

    std::optional<std::vector<bool>> valid = std::nullopt;
    if (has_validity_mask()) {
      valid = validity_mask()->to_vec<bool>();
    }

    std::vector<std::int64_t> indices(static_cast<std::size_t>(size()));
    std::iota(indices.begin(), indices.end(), static_cast<std::int64_t>(0));

    std::stable_sort(indices.begin(), indices.end(), [&](std::int64_t lhs, std::int64_t rhs) {
      if (valid.has_value()) {
        const bool lv = (*valid)[static_cast<std::size_t>(lhs)];
        const bool rv = (*valid)[static_cast<std::size_t>(rhs)];
        if (lv != rv) {
          return nulls_last ? lv : !lv;
        }
      }

      const auto& lk = keys[static_cast<std::size_t>(lhs)];
      const auto& rk = keys[static_cast<std::size_t>(rhs)];
      if (lk == rk) {
        return false;
      }
      return ascending ? (lk < rk) : (lk > rk);
    });

    return take(indices);
  }

  std::vector<std::int64_t> indices(static_cast<std::size_t>(size()));
  std::iota(indices.begin(), indices.end(), static_cast<std::int64_t>(0));

  std::stable_sort(indices.begin(), indices.end(), [&](std::int64_t lhs, std::int64_t rhs) {
    const auto lhs_valid = is_valid(lhs);
    const auto rhs_valid = is_valid(rhs);
    if (lhs_valid != rhs_valid) {
      if (nulls_last) {
        return lhs_valid;
      }
      return !lhs_valid;
    }

    const auto cmp = compare_rows(lhs, rhs, nulls_last);
    if (cmp == 0) {
      return false;
    }
    return ascending ? (cmp < 0) : (cmp > 0);
  });

  return take(indices);
}

Thread Thread::ffill(std::int64_t limit) const {
  return forward_fill_thread(*this, limit);
}

Thread Thread::bfill(std::int64_t limit) const {
  return backward_fill_thread(*this, limit);
}

Thread Thread::interp(std::int64_t limit) const {
  PLUSS_CHECK(limit == -1 || limit >= 0, "Thread::interp: limit must be -1 or >= 0");
  PLUSS_CHECK(kind() == ThreadKind::Series && is_numeric_series_type(as_series()),
              "Thread::interp is only valid for numeric Series payload");
  if (limit == 0 || size() == 0) {
    return *this;
  }
  auto valid = notnull().as_series().to_vec<bool>();
  if (all_true(valid)) {
    return *this;
  }

  const auto source = as_series().to(pluss::codem::ScalarType::Float64);
  const auto platform = as_series().platform();
  const auto values = source.to_vec<double>();
  std::vector<double> out = values;

  std::int64_t left = -1;
  for (std::int64_t i = 0; i < size(); ++i) {
    if (valid[static_cast<std::size_t>(i)]) {
      if (left >= 0 && i - left > 1) {
        const auto gap = i - left - 1;
        const auto fill_count = limit < 0 ? gap : std::min<std::int64_t>(gap, limit);
        const auto left_value = values[static_cast<std::size_t>(left)];
        const auto right_value = values[static_cast<std::size_t>(i)];
        for (std::int64_t k = 1; k <= fill_count; ++k) {
          const auto pos = left + k;
          const auto weight = static_cast<double>(k) / static_cast<double>(gap + 1);
          out[static_cast<std::size_t>(pos)] =
              left_value + (right_value - left_value) * weight;
          valid[static_cast<std::size_t>(pos)] = true;
        }
      }
      left = i;
    }
  }

  auto series = pluss::Series::empty(
      {size()}, pluss::codem::ScalarType::Float64, platform);
  if (!out.empty()) {
    auto cpu = pluss::Series::empty({size()}, pluss::codem::ScalarType::Float64);
    std::memcpy(cpu.raw_data(), out.data(), out.size() * sizeof(double));
    series = platform == pluss::codem::Platform::CPU ? cpu : cpu.to(platform);
  }
  if (all_true(valid)) {
    return Thread(series);
  }
  auto mask = bool_series_from_bools(valid);
  if (mask.platform() != platform) {
    mask = mask.to(platform);
  }
  return Thread(series, mask);
}

Thread Thread::bucketize(pluss::msharp::RangeView::Double bins,
                         pluss::msharp::RangeView::Int labels,
                         bool right,
                         bool include_lowest) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::bucketize is only valid for Series payload");
  PLUSS_CHECK(is_numeric_series_type(as_series()),
              "Thread::bucketize is only valid for numeric Series payload");

  std::vector<double> edges(bins.begin(), bins.end());
  PLUSS_CHECK(edges.size() >= 2, "Thread::bucketize: bins must contain at least two edges");
  for (std::size_t i = 0; i < edges.size(); ++i) {
    PLUSS_CHECK(!std::isnan(edges[i]),
                "Thread::bucketize: bin edges must not be NaN");
    if (i > 0) {
      PLUSS_CHECK(edges[i] > edges[i - 1],
                  "Thread::bucketize: bin edges must be strictly increasing");
    }
  }

  const auto bucket_count = static_cast<std::int64_t>(edges.size() - 1);
  std::vector<std::int64_t> label_values;
  if (labels.size() == 0) {
    label_values.resize(static_cast<std::size_t>(bucket_count));
    std::iota(label_values.begin(), label_values.end(), static_cast<std::int64_t>(0));
  } else {
    PLUSS_CHECK(static_cast<std::int64_t>(labels.size()) == bucket_count,
                "Thread::bucketize: labels size must equal bins.size() - 1");
    label_values.assign(labels.begin(), labels.end());
  }

  auto x = as_series().to(pluss::codem::ScalarType::Float64);
  const auto platform = x.platform();
  auto out = pluss::Series::full({x.numel()},
                                 static_cast<double>(label_values.front()),
                                 pluss::codem::ScalarType::Int64).to(platform);
  auto assigned = pluss::Series::zeros({x.numel()}, pluss::codem::ScalarType::Bool).to(platform);

  pluss::Series base_valid;
  if (has_validity_mask()) {
    base_valid = notnull().as_series();
    if (base_valid.platform() != platform) {
      base_valid = base_valid.to(platform);
    }
  } else {
    base_valid = pluss::Series::ones({x.numel()}, pluss::codem::ScalarType::Bool).to(platform);
  }

  for (std::int64_t i = 0; i < bucket_count; ++i) {
    const double left = edges[static_cast<std::size_t>(i)];
    const double right_edge = edges[static_cast<std::size_t>(i + 1)];

    pluss::Series lower;
    if (right) {
      lower = (i == 0 && include_lowest) ? x.ge(left) : x.gt(left);
    } else {
      lower = x.ge(left);
    }
    const auto upper = right ? x.le(right_edge) : x.lt(right_edge);
    auto mask = lower.logical_and(upper).logical_and(base_valid);

    out = out.where(mask, static_cast<double>(label_values[static_cast<std::size_t>(i)]));
    assigned = assigned.logical_or(mask);
  }

  const auto assigned_count = assigned.count().to(pluss::codem::Platform::CPU).item<std::int64_t>();
  if (assigned_count == x.numel()) {
    return Thread(out);
  }
  return Thread(out, assigned);
}

Thread Thread::distinct(bool dropnull) const {
  std::vector<std::int64_t> keep_indices;
  keep_indices.reserve(static_cast<std::size_t>(size()));

  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(size()));
  bool null_emitted = false;

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->to_vec<bool>();
  }

  for (std::int64_t i = 0; i < size(); ++i) {
    const bool is_valid = !valid.has_value() || (*valid)[static_cast<std::size_t>(i)];
    if (!is_valid) {
      if (!dropnull && !null_emitted) {
        keep_indices.push_back(i);
        null_emitted = true;
      }
      continue;
    }

    const auto key = thread_scalar_string_at(*this, i);
    if (seen.insert(key).second) {
      keep_indices.push_back(i);
    }
  }

  return take(keep_indices);
}

DataFrameView Thread::tally(std::int64_t top_n, bool dropnull, bool render) const {
  PLUSS_CHECK(top_n >= 0, "Thread::tally: top_n must be >= 0");

  struct TallyRow {
    std::string key;
    std::int64_t count{0};
  };

  std::vector<TallyRow> rows;
  rows.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, size() / 8)));
  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(rows.capacity());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->to_vec<bool>();
  }

  for (std::int64_t i = 0; i < size(); ++i) {
    const bool is_valid = !valid.has_value() || (*valid)[static_cast<std::size_t>(i)];
    if (!is_valid && dropnull) {
      continue;
    }
    const std::string key = is_valid ? thread_scalar_string_at(*this, i) : "NA";
    const auto [it, inserted] = index_by_key.emplace(key, rows.size());
    if (inserted) {
      rows.push_back(TallyRow{key, 1});
    } else {
      rows[it->second].count += 1;
    }
  }

  std::sort(rows.begin(), rows.end(), [](const TallyRow& lhs, const TallyRow& rhs) {
    if (lhs.count != rhs.count) {
      return lhs.count > rhs.count;
    }
    return lhs.key < rhs.key;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < rows.size()) {
    rows.resize(static_cast<std::size_t>(top_n));
  }

  std::vector<std::string> labels;
  labels.reserve(rows.size());
  std::vector<std::int64_t> counts;
  counts.reserve(rows.size());
  for (const auto& row : rows) {
    labels.push_back(row.key);
    counts.push_back(row.count);
  }

  std::vector<std::string_view> label_views;
  label_views.reserve(labels.size());
  for (const auto& value : labels) {
    label_views.push_back(value);
  }
  const std::array<std::int64_t, 1> label_shape{static_cast<std::int64_t>(label_views.size())};
  auto keys_note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(label_views.data(), label_views.size()),
      std::span<const std::int64_t>(label_shape.data(), label_shape.size()));

  auto count_series = pluss::Series::empty({static_cast<std::int64_t>(counts.size())},
                                           pluss::codem::ScalarType::Int64);
  if (!counts.empty()) {
    std::memcpy(count_series.raw_data(), counts.data(), counts.size() * sizeof(std::int64_t));
  }

  auto out = DataFrame({
      {"value", Thread(keys_note)},
      {"count", Thread(count_series)},
  });
  return DataFrameView(std::move(out), render, "tally", "");
}

Thread Thread::freq(std::int64_t top_n, bool dropnull) const {
  DataFrame frame = tally(top_n, dropnull, false);
  return frame.features("count").caselbl(frame.features("value")).with_display_name("freq");
}

ThreadValue Thread::ndistinct(bool dropnull) const {
  auto value = pluss::Series::empty({1}, pluss::codem::ScalarType::Int64);
  reinterpret_cast<std::int64_t*>(value.raw_data())[0] = distinct(dropnull).size();
  return ThreadValue(Thread(value));
}

Thread Thread::duplicated(bool keep_first, bool keep_last) const {
  PLUSS_CHECK(!(keep_first && keep_last),
              "Thread::duplicated: keep_first and keep_last cannot both be true");
  std::vector<bool> duplicated_mask(static_cast<std::size_t>(size()), false);
  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->to_vec<bool>();
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_key;
  rows_by_key.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t row = 0; row < size(); ++row) {
    const bool observed = !valid.has_value() || (*valid)[static_cast<std::size_t>(row)];
    const auto key = observed ? thread_scalar_string_at(*this, row) : std::string{"<NA>"};
    rows_by_key[key].push_back(row);
  }

  for (const auto& [_, rows] : rows_by_key) {
    if (rows.size() <= 1) {
      continue;
    }
    const std::size_t start = keep_first ? 1 : 0;
    const std::size_t stop = keep_last ? rows.size() - 1 : rows.size();
    for (std::size_t i = start; i < stop; ++i) {
      duplicated_mask[static_cast<std::size_t>(rows[i])] = true;
    }
  }
  return Thread(bool_series_from_bools(duplicated_mask));
}

ThreadView Thread::describe(bool render) const {
  const auto name = has_display_name() ? display_name() : std::string("value");

  if (kind() == ThreadKind::Series && is_numeric_series_type(as_series())) {
    auto desc_frame = DataFrame({{name, *this}}).describe(false);
    auto out = desc_frame.features(name);
    return ThreadView(std::move(out), render, "describe");
  }

  if (kind() == ThreadKind::Note) {
    const auto non_null = count(true).as_series().item<std::int64_t>();
    const auto distinct_count = distinct(/*dropnull=*/true).count(true).as_series().item<std::int64_t>();
    auto counts = tally(/*top_n=*/0, /*dropnull=*/true, /*render=*/false);

    std::string top_value = "NA";
    std::int64_t top_freq = 0;
    if (counts.ncases() > 0) {
      const auto top = counts.case_study(pluss::table::pos(0), pluss::table::cols({"value", "count"}));
      top_value = std::string(top.features("value").as_note().at(0));
      top_freq = std::stoll(thread_scalar_string_at(top.features("count"), 0));
    }

    std::vector<std::string> summary_values{
        std::to_string(non_null),
        std::to_string(distinct_count),
        top_value,
        std::to_string(top_freq),
    };

    std::vector<std::string_view> summary_views;
    summary_views.reserve(summary_values.size());
    for (const auto& v : summary_values) {
      summary_views.emplace_back(v);
    }
    const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(summary_views.size())};
    auto payload = pluss::coden::ops_internal::note_from_utf8(
        std::span<const std::string_view>(summary_views.data(), summary_views.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));

    auto out = Thread(payload)
                   .caselbl(Thread(make_utf8_note({"count", "unique", "top", "freq"})))
                   .with_display_name(name);
    return ThreadView(std::move(out), render, "describe");
  }

  PLUSS_FAIL("Thread::describe supports numeric Series and Note payloads");
}

ThreadValue Thread::sum() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::sum is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::sum on an empty (all-null) selection");
  return ThreadValue(Thread(s.sum().reshape({1})));
}

ThreadValue Thread::prod() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::prod is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::prod on an empty (all-null) selection");
  return ThreadValue(Thread(s.prod().reshape({1})));
}

ThreadValue Thread::count(bool count_true) const {
  if (kind() == ThreadKind::Series) {
    auto s = as_series();
    if (has_validity_mask()) {
      s = s.masked_select(notnull().as_series());
    }
    return ThreadValue(Thread(s.count(count_true).reshape({1})));
  }

  PLUSS_CHECK(count_true, "Thread::count(false) is only valid for Bool Series payload");
  std::int64_t total = size();
  if (has_validity_mask()) {
    const auto valid = notnull().as_series().to_vec<bool>();
    total = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  }
  auto out = pluss::Series::empty({1}, pluss::codem::ScalarType::Int64);
  reinterpret_cast<std::int64_t*>(out.raw_data())[0] = total;
  return ThreadValue(Thread(out));
}

ThreadValue Thread::any(bool skipna) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::any is only valid for Series payload");
  auto s = as_series();
  PLUSS_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool,
              "Thread::any is only valid for Bool Series payload");

  if (has_validity_mask()) {
    PLUSS_CHECK(skipna,
                "Thread::any(skipna=false) is not supported for nullable Bool Series payload");
    const auto valid = notnull().as_series();
    s = s.logical_and(valid);
  }

  const auto true_count = s.count(true).item<std::int64_t>();
  return ThreadValue(Thread(bool_series_from_bools({true_count > 0})));
}

ThreadValue Thread::all(bool skipna) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::all is only valid for Series payload");
  auto s = as_series();
  PLUSS_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool,
              "Thread::all is only valid for Bool Series payload");

  if (has_validity_mask()) {
    PLUSS_CHECK(skipna,
                "Thread::all(skipna=false) is not supported for nullable Bool Series payload");
    const auto valid = notnull().as_series();
    s = s.logical_or(valid.logical_not());
  }

  const auto true_count = s.count(true).item<std::int64_t>();
  return ThreadValue(Thread(bool_series_from_bools({true_count == s.numel()})));
}

ThreadValue Thread::mean() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::mean is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(
        s.platform() == pluss::codem::Platform::CPU
            ? pluss::codem::ScalarType::Float64
            : pluss::codem::ScalarType::Float32);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::mean on an empty (all-null) selection");
  return ThreadValue(Thread(s.mean().reshape({1})));
}

ThreadValue Thread::median() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::median is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(
        s.platform() == pluss::codem::Platform::CPU
            ? pluss::codem::ScalarType::Float64
            : pluss::codem::ScalarType::Float32);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::median on an empty (all-null) selection");
  return ThreadValue(Thread(s.median().reshape({1})));
}

ThreadValue Thread::min() const {
  auto observed = has_validity_mask() ? dropnull() : *this;
  PLUSS_CHECK(observed.size() > 0, "Thread::min on an empty (all-null) selection");
  switch (observed.kind()) {
    case ThreadKind::Series:
      return ThreadValue(Thread(observed.as_series().min().reshape({1})));
    case ThreadKind::Note: {
      const auto sorted = observed.sort_values(true, true);
      return ThreadValue(Thread(make_utf8_note({std::string(sorted.as_note().at(0))})));
    }
    case ThreadKind::Timeline: {
      const auto sorted = observed.sort_values(true, true);
      const auto ticks = sorted.as_timeline().to_epoch_nanos_vector();
      return ThreadValue(Thread(rebuild_timeline_like(sorted.as_timeline(), {ticks.front()})));
    }
    case ThreadKind::TimeIndex: {
      const auto sorted = observed.sort_values(true, true);
      const auto ticks = sorted.as_timeindex().timeline().to_epoch_nanos_vector();
      auto timeline = rebuild_timeline_like(sorted.as_timeindex().timeline(), {ticks.front()});
      return ThreadValue(Thread(timeline.to_timeindex(false)));
    }
  }
  PLUSS_FAIL("Thread::min: unsupported payload kind");
}

ThreadValue Thread::max() const {
  auto observed = has_validity_mask() ? dropnull() : *this;
  PLUSS_CHECK(observed.size() > 0, "Thread::max on an empty (all-null) selection");
  switch (observed.kind()) {
    case ThreadKind::Series:
      return ThreadValue(Thread(observed.as_series().max().reshape({1})));
    case ThreadKind::Note: {
      const auto sorted = observed.sort_values(false, true);
      return ThreadValue(Thread(make_utf8_note({std::string(sorted.as_note().at(0))})));
    }
    case ThreadKind::Timeline: {
      const auto sorted = observed.sort_values(false, true);
      const auto ticks = sorted.as_timeline().to_epoch_nanos_vector();
      return ThreadValue(Thread(rebuild_timeline_like(sorted.as_timeline(), {ticks.front()})));
    }
    case ThreadKind::TimeIndex: {
      const auto sorted = observed.sort_values(false, true);
      const auto ticks = sorted.as_timeindex().timeline().to_epoch_nanos_vector();
      auto timeline = rebuild_timeline_like(sorted.as_timeindex().timeline(), {ticks.front()});
      return ThreadValue(Thread(timeline.to_timeindex(false)));
    }
  }
  PLUSS_FAIL("Thread::max: unsupported payload kind");
}

ThreadValue Thread::csmin() const {
  PLUSS_CHECK(size() > 0, "Thread::csmin on an empty Thread");
  std::int64_t best = -1;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    if (best < 0 || compare_rows(row, best, true) < 0) {
      best = row;
    }
  }
  PLUSS_CHECK(best >= 0, "Thread::csmin on an all-null Thread");
  const std::array<std::int64_t, 1> index{best};
  return ThreadValue(caselbl().take(std::span<const std::int64_t>(index.data(), index.size())));
}

ThreadValue Thread::csmax() const {
  PLUSS_CHECK(size() > 0, "Thread::csmax on an empty Thread");
  std::int64_t best = -1;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    if (best < 0 || compare_rows(row, best, true) > 0) {
      best = row;
    }
  }
  PLUSS_CHECK(best >= 0, "Thread::csmax on an all-null Thread");
  const std::array<std::int64_t, 1> index{best};
  return ThreadValue(caselbl().take(std::span<const std::int64_t>(index.data(), index.size())));
}

ThreadValue Thread::var(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::var is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(
        s.platform() == pluss::codem::Platform::CPU
            ? pluss::codem::ScalarType::Float64
            : pluss::codem::ScalarType::Float32);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::var on an empty (all-null) selection");
  return ThreadValue(Thread(s.var(unbiased).reshape({1})));
}

ThreadValue Thread::std(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::std is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(
        s.platform() == pluss::codem::Platform::CPU
            ? pluss::codem::ScalarType::Float64
            : pluss::codem::ScalarType::Float32);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::std on an empty (all-null) selection");
  return ThreadValue(Thread(s.std(unbiased).reshape({1})));
}

double Thread::corr(const Thread& other, ThreadCorrMethod method) const {
  PLUSS_CHECK(kind() == ThreadKind::Series && other.kind() == ThreadKind::Series,
              "Thread::corr: both operands must be Series");
  PLUSS_CHECK(is_numeric_series_type(as_series()) && is_numeric_series_type(other.as_series()),
              "Thread::corr: both operands must be numeric Series");
  const auto aligned = align_thread_pair_for_cases_(*this, other, "Thread::corr");
  if (aligned.aligned) {
    return aligned.left.corr(aligned.right, method);
  }

  auto lhs = as_series().to(pluss::codem::ScalarType::Float64);
  auto rhs = other.as_series().to(pluss::codem::ScalarType::Float64);

  auto valid = lhs.isfinite().logical_and(rhs.isfinite());
  if (has_validity_mask()) {
    valid = valid.logical_and(notnull().as_series());
  }
  if (other.has_validity_mask()) {
    valid = valid.logical_and(other.notnull().as_series());
  }

  const auto lhs_valid = lhs.masked_select(valid);
  const auto rhs_valid = rhs.masked_select(valid);
  if (lhs_valid.numel() < 2 || rhs_valid.numel() < 2 || lhs_valid.numel() != rhs_valid.numel()) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  switch (method) {
    case ThreadCorrMethod::Pearson: {
      // Vectorized Pearson path: keep heavy math in Series kernels.
      const auto mean_l = lhs_valid.mean().item<double>();
      const auto mean_r = rhs_valid.mean().item<double>();
      const auto dl = lhs_valid - mean_l;
      const auto dr = rhs_valid - mean_r;
      const auto sxy = (dl * dr).sum().item<double>();
      const auto sxx = (dl * dl).sum().item<double>();
      const auto syy = (dr * dr).sum().item<double>();

      if (!(sxx > 0.0) || !(syy > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      const double corr = sxy / std::sqrt(sxx * syy);
      if (!std::isfinite(corr)) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      return std::clamp(corr, -1.0, 1.0);
    }
    case ThreadCorrMethod::Spearman: {
      const auto x = lhs_valid.to_vec<double>();
      const auto y = rhs_valid.to_vec<double>();
      const auto rx = rank_average_ties_(x);
      const auto ry = rank_average_ties_(y);
      return pearson_corr_from_vectors_(rx, ry);
    }
    case ThreadCorrMethod::KendallTau: {
      const auto x = lhs_valid.to_vec<double>();
      const auto y = rhs_valid.to_vec<double>();
      return kendall_tau_b_from_vectors_(x, y);
    }
    default: {
      const auto mean_l = lhs_valid.mean().item<double>();
      const auto mean_r = rhs_valid.mean().item<double>();
      const auto dl = lhs_valid - mean_l;
      const auto dr = rhs_valid - mean_r;
      const auto sxy = (dl * dr).sum().item<double>();
      const auto sxx = (dl * dl).sum().item<double>();
      const auto syy = (dr * dr).sum().item<double>();
      if (!(sxx > 0.0) || !(syy > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      const double corr = sxy / std::sqrt(sxx * syy);
      if (!std::isfinite(corr)) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      return std::clamp(corr, -1.0, 1.0);
    }
  }
}

Thread Thread::lower() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::lower is only valid for Note payload");
  return Thread(as_note().lower(), validity_mask());
}

Thread Thread::strip() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::strip is only valid for Note payload");
  return Thread(as_note().strip(), validity_mask());
}

Thread Thread::strip(std::string_view chars) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::strip(chars) is only valid for Note payload");
  return Thread(as_note().strip(chars), validity_mask());
}

Thread Thread::lstrip() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::lstrip is only valid for Note payload");
  return Thread(as_note().lstrip(), validity_mask());
}

Thread Thread::lstrip(std::string_view chars) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::lstrip(chars) is only valid for Note payload");
  return Thread(as_note().lstrip(chars), validity_mask());
}

Thread Thread::rstrip() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::rstrip is only valid for Note payload");
  return Thread(as_note().rstrip(), validity_mask());
}

Thread Thread::rstrip(std::string_view chars) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::rstrip(chars) is only valid for Note payload");
  return Thread(as_note().rstrip(chars), validity_mask());
}

namespace {

std::optional<pluss::Series> nullify_matches_mask(const Thread& t, const pluss::Series& matches, bool& changed) {
  auto valid = detail::has_validity_mask(t) ? detail::validity_mask(t)->to_vec<bool>()
                                           : std::vector<bool>(static_cast<std::size_t>(t.size()), true);
  const auto hit = matches.to_vec<bool>();

  changed = false;
  for (std::int64_t i = 0; i < t.size(); ++i) {
    const auto idx = static_cast<std::size_t>(i);
    if (valid[idx] && hit[idx]) {
      valid[idx] = false;
      changed = true;
    }
  }

  if (!changed) {
    return detail::validity_mask(t);
  }
  return optional_mask_from_valid(valid);
}

} // namespace

Thread Thread::replace(std::string_view from, std::string_view to) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::replace is only valid for Note payload");
  return Thread(as_note().replace(from, to), validity_mask());
}

Thread Thread::replace(double from, double to) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::replace(double, double) is only valid for Series payload");
  auto mask = compare(from, ThreadBinaryCompareOp::Eq).as_series();
  return Thread(as_series().where(mask, to), validity_mask());
}

Thread Thread::replace(double from, pluss::null_t) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::replace(double, null) is only valid for Series payload");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, compare(from, ThreadBinaryCompareOp::Eq).as_series(), changed);
  if (!changed) {
    return *this;
  }
  return Thread(as_series(), new_mask);
}

Thread Thread::replace(pluss::null_t, double to) const {
  return fillna(to);
}

Thread Thread::replace(const pluss::Series& from, const pluss::Series& to) const {
  PLUSS_CHECK(from.defined(), "Thread::replace(Series, Series): from Series is undefined");
  PLUSS_CHECK(to.defined(), "Thread::replace(Series, Series): to Series is undefined");
  PLUSS_CHECK(from.numel() == 1, "Thread::replace(Series, Series): from Series must be scalar");
  PLUSS_CHECK(to.numel() == 1, "Thread::replace(Series, Series): to Series must be scalar");
  double lhs = 0.0;
  double rhs = 0.0;
  PLUSS_CHECK(extract_numeric_value(from, 0, lhs),
              "Thread::replace(Series, Series): from Series must be numeric or bool");
  PLUSS_CHECK(extract_numeric_value(to, 0, rhs),
              "Thread::replace(Series, Series): to Series must be numeric or bool");
  return replace(lhs, rhs);
}

Thread Thread::replace(const pluss::Series& from, pluss::null_t to) const {
  PLUSS_CHECK(from.defined(), "Thread::replace(Series, null): from Series is undefined");
  PLUSS_CHECK(from.numel() == 1, "Thread::replace(Series, null): from Series must be scalar");
  double lhs = 0.0;
  PLUSS_CHECK(extract_numeric_value(from, 0, lhs),
              "Thread::replace(Series, null): from Series must be numeric or bool");
  return replace(lhs, to);
}

Thread Thread::replace(pluss::null_t from, const pluss::Series& to) const {
  PLUSS_CHECK(to.defined(), "Thread::replace(null, Series): to Series is undefined");
  PLUSS_CHECK(to.numel() == 1, "Thread::replace(null, Series): to Series must be scalar");
  double rhs = 0.0;
  PLUSS_CHECK(extract_numeric_value(to, 0, rhs),
              "Thread::replace(null, Series): to Series must be numeric or bool");
  return replace(from, rhs);
}

Thread Thread::replace(const pluss::Note& from, const pluss::Note& to) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::replace(Note, Note) is only valid for Note payload");
  PLUSS_CHECK(from.defined(), "Thread::replace(Note, Note): from Note is undefined");
  PLUSS_CHECK(to.defined(), "Thread::replace(Note, Note): to Note is undefined");
  PLUSS_CHECK(from.numel() == 1, "Thread::replace(Note, Note): from Note must be scalar");
  PLUSS_CHECK(to.numel() == 1, "Thread::replace(Note, Note): to Note must be scalar");

  const auto from_text = std::string(from.at(0));
  const auto to_text = std::string(to.at(0));
  const auto valid = has_validity_mask() ? validity_mask()->to_vec<bool>()
                                         : std::vector<bool>(static_cast<std::size_t>(size()), true);
  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(size()));
  bool changed = false;
  for (std::int64_t i = 0; i < size(); ++i) {
    auto text = std::string(as_note().at(i));
    if (valid[static_cast<std::size_t>(i)] && text == from_text) {
      text = to_text;
      changed = true;
    }
    values.push_back(std::move(text));
  }
  if (!changed) {
    return *this;
  }
  return Thread(rebuild_note_like(as_note(), values), validity_mask());
}

Thread Thread::replace(const pluss::Note& from, pluss::null_t to) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::replace(Note, null) is only valid for Note payload");
  PLUSS_CHECK(from.defined(), "Thread::replace(Note, null): from Note is undefined");
  PLUSS_CHECK(from.numel() == 1, "Thread::replace(Note, null): from Note must be scalar");

  const auto from_text = std::string(from.at(0));
  auto valid = has_validity_mask() ? validity_mask()->to_vec<bool>()
                                   : std::vector<bool>(static_cast<std::size_t>(size()), true);
  bool changed = false;
  for (std::int64_t i = 0; i < size(); ++i) {
    const auto idx = static_cast<std::size_t>(i);
    if (valid[idx] && as_note().at(i) == from_text) {
      valid[idx] = false;
      changed = true;
    }
  }
  if (!changed) {
    return *this;
  }
  return Thread(as_note(), optional_mask_from_valid(valid));
}

Thread Thread::replace(pluss::null_t, const pluss::Note& to) const {
  PLUSS_CHECK(to.defined(), "Thread::replace(null, Note): to Note is undefined");
  PLUSS_CHECK(to.numel() == 1, "Thread::replace(null, Note): to Note must be scalar");
  return fillna(to.at(0));
}

Thread Thread::replace(const pluss::Timeline& from, const pluss::Timeline& to) const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex,
              "Thread::replace(Timeline, Timeline) is only valid for Timeline/TimeIndex payload");
  PLUSS_CHECK(from.defined(), "Thread::replace(Timeline, Timeline): from Timeline is undefined");
  PLUSS_CHECK(to.defined(), "Thread::replace(Timeline, Timeline): to Timeline is undefined");
  PLUSS_CHECK(from.numel() == 1, "Thread::replace(Timeline, Timeline): from Timeline must be scalar");
  PLUSS_CHECK(to.numel() == 1, "Thread::replace(Timeline, Timeline): to Timeline must be scalar");

  const auto keep = compare(from, ThreadBinaryCompareOp::Eq).as_series().logical_not();
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().where(keep, to), validity_mask());
  }
  return Thread(as_timeindex().timeline().where(keep, to).to_timeindex(false), validity_mask());
}

Thread Thread::replace(const pluss::Timeline& from, pluss::null_t to) const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex,
              "Thread::replace(Timeline, null) is only valid for Timeline/TimeIndex payload");
  PLUSS_CHECK(from.defined(), "Thread::replace(Timeline, null): from Timeline is undefined");
  PLUSS_CHECK(from.numel() == 1, "Thread::replace(Timeline, null): from Timeline must be scalar");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, compare(from, ThreadBinaryCompareOp::Eq).as_series(), changed);
  if (!changed) {
    return *this;
  }
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline(), new_mask);
  }
  return Thread(as_timeindex(), new_mask);
}

Thread Thread::replace(pluss::null_t, const pluss::Timeline& to) const {
  return fillna(to);
}

Thread Thread::replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to) const {
  PLUSS_CHECK(kind() == ThreadKind::TimeIndex,
              "Thread::replace(TimeIndex, TimeIndex) is only valid for TimeIndex payload");
  PLUSS_CHECK(from.timeline().defined(), "Thread::replace(TimeIndex, TimeIndex): from TimeIndex is undefined");
  PLUSS_CHECK(to.timeline().defined(), "Thread::replace(TimeIndex, TimeIndex): to TimeIndex is undefined");
  PLUSS_CHECK(from.size() == 1, "Thread::replace(TimeIndex, TimeIndex): from TimeIndex must be scalar");
  PLUSS_CHECK(to.size() == 1, "Thread::replace(TimeIndex, TimeIndex): to TimeIndex must be scalar");

  const auto keep = compare(from, ThreadBinaryCompareOp::Eq).as_series().logical_not();
  return Thread(as_timeindex().timeline().where(keep, to.timeline()).to_timeindex(false), validity_mask());
}

Thread Thread::replace(const pluss::TimeIndex& from, pluss::null_t to) const {
  PLUSS_CHECK(kind() == ThreadKind::TimeIndex,
              "Thread::replace(TimeIndex, null) is only valid for TimeIndex payload");
  PLUSS_CHECK(from.timeline().defined(), "Thread::replace(TimeIndex, null): from TimeIndex is undefined");
  PLUSS_CHECK(from.size() == 1, "Thread::replace(TimeIndex, null): from TimeIndex must be scalar");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, compare(from, ThreadBinaryCompareOp::Eq).as_series(), changed);
  if (!changed) {
    return *this;
  }
  return Thread(as_timeindex(), new_mask);
}

Thread Thread::replace(pluss::null_t, const pluss::TimeIndex& to) const {
  return fillna(to);
}

Thread Thread::has(double value) const {
  return eq(value);
}

Thread Thread::has(std::string_view value) const {
  return eq(value);
}

Thread Thread::has(const Thread& values) const {
  PLUSS_CHECK(values.defined(), "Thread::has: values Thread is undefined");
  auto out = Thread(pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool));
  if (detail::has_display_labels(*this)) {
    out = detail::with_display_labels(out, detail::display_labels(*this));
  }
  for (std::int64_t i = 0; i < values.size(); ++i) {
    if (!values.is_valid(i)) {
      continue;
    }
    if (values.kind() == ThreadKind::Series && is_numeric_series_type(values.as_series())) {
      double scalar = 0.0;
      PLUSS_CHECK(extract_numeric_value(values.as_series(), i, scalar),
                  "Thread::has: failed to extract numeric membership value");
      out = out.lor(has(scalar));
    } else {
      out = out.lor(has(thread_scalar_string_at(values, i)));
    }
  }
  return out;
}

Thread Thread::year() const {
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().year(), validity_mask());
  }
  if (kind() == ThreadKind::TimeIndex) {
    return Thread(as_timeindex().timeline().year(), validity_mask());
  }
  PLUSS_FAIL("Thread::year is only valid for Timeline/TimeIndex payload");
}

Thread Thread::month() const {
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().month(), validity_mask());
  }
  if (kind() == ThreadKind::TimeIndex) {
    return Thread(as_timeindex().timeline().month(), validity_mask());
  }
  PLUSS_FAIL("Thread::month is only valid for Timeline/TimeIndex payload");
}

Thread Thread::day() const {
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().day(), validity_mask());
  }
  if (kind() == ThreadKind::TimeIndex) {
    return Thread(as_timeindex().timeline().day(), validity_mask());
  }
  PLUSS_FAIL("Thread::day is only valid for Timeline/TimeIndex payload");
}

Thread Thread::hour() const {
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().hour(), validity_mask());
  }
  if (kind() == ThreadKind::TimeIndex) {
    return Thread(as_timeindex().timeline().hour(), validity_mask());
  }
  PLUSS_FAIL("Thread::hour is only valid for Timeline/TimeIndex payload");
}

Thread Thread::to_note_weekday() const {
  if (kind() == ThreadKind::Timeline) {
    return Thread(as_timeline().to_note_weekday(), validity_mask());
  }
  if (kind() == ThreadKind::TimeIndex) {
    return Thread(as_timeindex().timeline().to_note_weekday(), validity_mask());
  }
  PLUSS_FAIL("Thread::to_note_weekday is only valid for Timeline/TimeIndex payload");
}

Thread Thread::numeric_binary(double other, ThreadBinaryMathOp op, bool reverse) const {
  if (kind() != ThreadKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  return carry_case_labels_for_alignment_(
      Thread(apply_math_series_scalar(as_series(), other, op, reverse), validity_mask()),
      *this);
}

Thread Thread::numeric_binary(const Thread& other, ThreadBinaryMathOp op) const {
  if (kind() != ThreadKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  PLUSS_CHECK(other.kind() == ThreadKind::Series && is_numeric_series_type(other.as_series()),
              "Thread::numeric_binary: rhs must be numeric Series");
  return aligned_numeric_binary_(*this, other, op, "Thread::numeric_binary");
}

Thread Thread::compare(double other, ThreadBinaryCompareOp op) const {
  if (kind() == ThreadKind::Series) {
    return carry_case_labels_for_alignment_(
        Thread(apply_compare_series_scalar(as_series(), other, op), validity_mask()),
        *this);
  }

  auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->to_vec<bool>();
  }

  for (std::int64_t i = 0; i < size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      p[i] = false;
      continue;
    }
    const auto text = thread_scalar_string_at(*this, i);
    double num = 0.0;
    bool bv = false;
    if (parse_double_exact(text, num)) {
      p[i] = compare_numbers(num, other, op);
    } else if (parse_bool_like(text, bv)) {
      p[i] = compare_numbers(bv ? 1.0 : 0.0, other, op);
    } else {
      p[i] = false;
    }
  }
  return carry_case_labels_for_alignment_(Thread(out, validity_mask()), *this);
}

Thread Thread::compare(std::string_view other, ThreadBinaryCompareOp op) const {
  if (kind() == ThreadKind::Note) {
    std::vector<std::uint8_t> raw;
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        raw = (as_note() == other);
        break;
      case ThreadBinaryCompareOp::Ne:
        raw = (as_note() != other);
        break;
      case ThreadBinaryCompareOp::Lt:
        raw = (as_note() < other);
        break;
      case ThreadBinaryCompareOp::Le:
        raw = (as_note() <= other);
        break;
      case ThreadBinaryCompareOp::Gt:
        raw = (as_note() > other);
        break;
      case ThreadBinaryCompareOp::Ge:
        raw = (as_note() >= other);
        break;
    }
    return carry_case_labels_for_alignment_(Thread(bool_series_from_u8(raw), validity_mask()), *this);
  }

  auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->to_vec<bool>();
  }

  const std::string rhs_text(other);
  for (std::int64_t i = 0; i < size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      p[i] = false;
      continue;
    }
    const auto lhs_text = thread_scalar_string_at(*this, i);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        p[i] = lhs_text == rhs_text;
        break;
      case ThreadBinaryCompareOp::Ne:
        p[i] = lhs_text != rhs_text;
        break;
      case ThreadBinaryCompareOp::Lt:
        p[i] = lhs_text < rhs_text;
        break;
      case ThreadBinaryCompareOp::Le:
        p[i] = lhs_text <= rhs_text;
        break;
      case ThreadBinaryCompareOp::Gt:
        p[i] = lhs_text > rhs_text;
        break;
      case ThreadBinaryCompareOp::Ge:
        p[i] = lhs_text >= rhs_text;
        break;
    }
  }
  return carry_case_labels_for_alignment_(Thread(out, validity_mask()), *this);
}

Thread Thread::compare(const pluss::Series& other, ThreadBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Thread::compare(Series): rhs Series is undefined");
  if (other.dim() == 0) {
    double rhs_value = 0.0;
    PLUSS_CHECK(extract_numeric_value(other, 0, rhs_value),
                "Thread::compare(Series): scalar rhs Series must be numeric or bool");
    return compare(rhs_value, op);
  }
  PLUSS_CHECK(other.dim() == 1, "Thread::compare(Series): rhs Series must be scalar or 1D");
  if (other.numel() != size()) {
    throw ::pluss::Error("Thread::compare(Series): positional comparison requires identical lengths");
  }
  return compare(Thread(other), op);
}

Thread Thread::compare(const pluss::Note& other, ThreadBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Thread::compare(Note): rhs Note is undefined");
  if (other.dim() == 0) {
    return compare(other.at(0), op);
  }
  PLUSS_CHECK(other.dim() == 1, "Thread::compare(Note): rhs Note must be scalar or 1D");
  if (other.numel() != size()) {
    throw ::pluss::Error("Thread::compare(Note): positional comparison requires identical lengths");
  }
  return compare(Thread(other), op);
}

Thread Thread::compare(const pluss::Timeline& other, ThreadBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Thread::compare(Timeline): rhs Timeline is undefined");
  if (other.dim() == 0 && (kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex)) {
    const auto& lhs = (kind() == ThreadKind::Timeline) ? as_timeline() : as_timeindex().timeline();
    return carry_case_labels_for_alignment_(
        Thread(apply_compare_timeline_timeline(lhs, other, op), validity_mask()),
        *this);
  }
  PLUSS_CHECK(other.dim() == 1, "Thread::compare(Timeline): rhs Timeline must be scalar or 1D");
  if (other.numel() != size()) {
    throw ::pluss::Error("Thread::compare(Timeline): positional comparison requires identical lengths");
  }
  return compare(Thread(other), op);
}

Thread Thread::compare(const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const {
  PLUSS_CHECK(other.timeline().defined(), "Thread::compare(TimeIndex): rhs TimeIndex is undefined");
  if (other.timeline().dim() == 0 && (kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex)) {
    const auto& lhs = (kind() == ThreadKind::Timeline) ? as_timeline() : as_timeindex().timeline();
    return carry_case_labels_for_alignment_(
        Thread(
            apply_compare_timeline_timeline(lhs, other.timeline(), op),
            validity_mask()),
        *this);
  }
  if (other.size() != size()) {
    throw ::pluss::Error("Thread::compare(TimeIndex): positional comparison requires identical lengths");
  }
  return compare(Thread(other), op);
}

Thread Thread::compare(const Thread& other, ThreadBinaryCompareOp op) const {
  if (other.size() != size()) {
    throw ::pluss::Error("Thread::compare: positional alignment requires identical lengths");
  }
  return aligned_compare_(*this, other, op, "Thread::compare");
}

Thread Thread::btw(double a, double b, bool inclusive) const {
  return btw(a, b, inclusive, inclusive);
}

Thread Thread::btw(double a, double b, bool include_left, bool include_right) const {
  PLUSS_CHECK(kind() == ThreadKind::Series,
              "Thread::btw(double,double,...): requires Series payload; use temporal overloads for date/time threads");
  PLUSS_CHECK(is_numeric_series_type(as_series()),
              "Thread::btw(double,double,...): Series payload must be numeric");

  auto lower = a;
  auto upper = b;
  if (lower > upper) {
    std::swap(lower, upper);
    std::swap(include_left, include_right);
  }

  const auto left = include_left ? compare(lower, ThreadBinaryCompareOp::Ge)
                                 : compare(lower, ThreadBinaryCompareOp::Gt);
  const auto right = include_right ? compare(upper, ThreadBinaryCompareOp::Le)
                                   : compare(upper, ThreadBinaryCompareOp::Lt);
  return (left & right);
}

Thread Thread::btw(std::string_view lo, std::string_view hi, bool inclusive) const {
  return btw(lo, hi, inclusive, inclusive);
}

Thread Thread::btw(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex,
              "Thread::btw(string,string,...): requires Timeline/TimeIndex payload");

  auto lower = pluss::make_timeline(lo);
  auto upper = pluss::make_timeline(hi);
  if (lower > upper) {
    std::swap(lower, upper);
    std::swap(include_left, include_right);
  }

  const auto& axis = (kind() == ThreadKind::Timeline) ? as_timeline() : as_timeindex().timeline();
  const auto left = include_left ? axis.ge(lower) : axis.gt(lower);
  const auto right = include_right ? axis.le(upper) : axis.lt(upper);
  return Thread(left.logical_and(right), validity_mask());
}

Thread Thread::btw(const Thread& lo, const Thread& hi, bool inclusive) const {
  return btw(lo, hi, inclusive, inclusive);
}

Thread Thread::btw(const Thread& lo, const Thread& hi, bool include_left, bool include_right) const {
  PLUSS_CHECK(lo.defined() && hi.defined(), "Thread::btw(Thread,Thread,...): lower/upper bounds must be defined");
  const auto left = include_left ? (*this >= lo) : (*this > lo);
  const auto right = include_right ? (*this <= hi) : (*this < hi);
  return (left & right);
}

int Thread::compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last) const {
  const auto l = normalize_index(lhs_row, size());
  const auto r = normalize_index(rhs_row, size());

  const bool lv = is_valid(l);
  const bool rv = is_valid(r);
  if (!lv || !rv) {
    if (!lv && !rv) {
      return 0;
    }
    if (nulls_last) {
      return lv ? -1 : 1;
    }
    return lv ? 1 : -1;
  }

  auto cmp_numeric = [](double a, double b) -> int {
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
    return 0;
  };
  auto cmp_string = [](const std::string& a, const std::string& b) -> int {
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
    return 0;
  };

  switch (kind()) {
    case ThreadKind::Series: {
      const auto& s = as_series();
      if (is_numeric_series_type(s)) {
        double lv_num = 0.0;
        double rv_num = 0.0;
        const bool ok_l = extract_numeric_value(s, l, lv_num);
        const bool ok_r = extract_numeric_value(s, r, rv_num);
        if (ok_l && ok_r) {
          return cmp_numeric(lv_num, rv_num);
        }
      }
      return cmp_string(thread_scalar_string_at(*this, l), thread_scalar_string_at(*this, r));
    }
    case ThreadKind::Note:
      return cmp_string(std::string(as_note().at(l)), std::string(as_note().at(r)));
    case ThreadKind::Timeline: {
      const auto ticks = as_timeline().to_epoch_nanos_vector();
      const auto lv_tick = ticks[static_cast<std::size_t>(l)];
      const auto rv_tick = ticks[static_cast<std::size_t>(r)];
      if (lv_tick < rv_tick) {
        return -1;
      }
      if (lv_tick > rv_tick) {
        return 1;
      }
      return 0;
    }
    case ThreadKind::TimeIndex: {
      const auto ticks = as_timeindex().timeline().to_epoch_nanos_vector();
      const auto lv_tick = ticks[static_cast<std::size_t>(l)];
      const auto rv_tick = ticks[static_cast<std::size_t>(r)];
      if (lv_tick < rv_tick) {
        return -1;
      }
      if (lv_tick > rv_tick) {
        return 1;
      }
      return 0;
    }
    default:
      return 0;
  }
}

Thread Thread::window(std::int64_t window_size, ThreadWindowOp op, std::int64_t min_periods) const {
  PLUSS_CHECK(window_size > 0, "Thread::window: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "Thread::window: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window_size, "Thread::window: min_periods cannot exceed window");
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::window: only Series payload supports rolling ops");
  PLUSS_CHECK(is_numeric_series_type(as_series()), "Thread::window: Series payload must be numeric");

  const auto n = size();
  auto out = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  auto* out_data = reinterpret_cast<double*>(out.raw_data());
  std::vector<bool> out_valid(static_cast<std::size_t>(n), false);

  std::vector<bool> in_valid(static_cast<std::size_t>(n), true);
  if (has_validity_mask()) {
    in_valid = validity_mask()->to_vec<bool>();
  }

  std::vector<double> values(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    if (!in_valid[static_cast<std::size_t>(i)]) {
      continue;
    }
    double v = 0.0;
    const bool ok = extract_numeric_value(as_series(), i, v);
    PLUSS_CHECK(ok, "Thread::window: failed numeric extraction");
    values[static_cast<std::size_t>(i)] = v;
  }

  std::vector<double> prefix_sum(static_cast<std::size_t>(n + 1), 0.0);
  std::vector<std::int64_t> prefix_count(static_cast<std::size_t>(n + 1), 0);
  for (std::int64_t i = 0; i < n; ++i) {
    prefix_sum[static_cast<std::size_t>(i + 1)] =
        prefix_sum[static_cast<std::size_t>(i)] + (in_valid[static_cast<std::size_t>(i)] ? values[static_cast<std::size_t>(i)] : 0.0);
    prefix_count[static_cast<std::size_t>(i + 1)] =
        prefix_count[static_cast<std::size_t>(i)] + (in_valid[static_cast<std::size_t>(i)] ? 1 : 0);
  }

  auto valid_count_in_window = [&](std::int64_t start, std::int64_t stop_exclusive) {
    return prefix_count[static_cast<std::size_t>(stop_exclusive)] - prefix_count[static_cast<std::size_t>(start)];
  };

  if (op == ThreadWindowOp::RollingSum || op == ThreadWindowOp::RollingMean) {
    for (std::int64_t i = 0; i < n; ++i) {
      const auto start = std::max<std::int64_t>(0, i - window_size + 1);
      const auto stop = i + 1;
      const auto cnt = valid_count_in_window(start, stop);
      if (cnt < min_periods) {
        out_data[i] = 0.0;
        continue;
      }
      const auto sum =
          prefix_sum[static_cast<std::size_t>(stop)] - prefix_sum[static_cast<std::size_t>(start)];
      out_data[i] = (op == ThreadWindowOp::RollingMean)
                        ? (sum / static_cast<double>(cnt))
                        : sum;
      out_valid[static_cast<std::size_t>(i)] = true;
    }
    return Thread(out, optional_mask_from_valid(out_valid));
  }

  const bool is_min = (op == ThreadWindowOp::RollingMin);
  std::deque<std::int64_t> mono;
  for (std::int64_t i = 0; i < n; ++i) {
    if (in_valid[static_cast<std::size_t>(i)]) {
      while (!mono.empty()) {
        const auto back_idx = mono.back();
        const auto lhs = values[static_cast<std::size_t>(back_idx)];
        const auto rhs = values[static_cast<std::size_t>(i)];
        const bool pop = is_min ? (lhs >= rhs) : (lhs <= rhs);
        if (!pop) {
          break;
        }
        mono.pop_back();
      }
      mono.push_back(i);
    }

    const auto start = std::max<std::int64_t>(0, i - window_size + 1);
    while (!mono.empty() && mono.front() < start) {
      mono.pop_front();
    }

    const auto cnt = valid_count_in_window(start, i + 1);
    if (cnt < min_periods || mono.empty()) {
      out_data[i] = 0.0;
      continue;
    }
    out_data[i] = values[static_cast<std::size_t>(mono.front())];
    out_valid[static_cast<std::size_t>(i)] = true;
  }

  return Thread(out, optional_mask_from_valid(out_valid));
}

namespace {

std::int64_t cast_days_from_civil_(std::int64_t y, unsigned m, unsigned d) noexcept {
  y -= m <= 2;
  const auto era = (y >= 0 ? y : y - 399) / 400;
  const auto yoe = static_cast<unsigned>(y - era * 400);
  const auto doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const auto doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

std::optional<std::int64_t> cast_timeline_tick_from_pattern_(
    std::string_view text,
    std::string_view pattern) {
  std::tm tm{};
  tm.tm_mday = 1;
  std::istringstream input{std::string(text)};
  const auto fmt = std::string(pattern);
  input >> std::get_time(&tm, fmt.c_str());
  if (input.fail()) {
    return std::nullopt;
  }
  input >> std::ws;
  if (!input.eof()) {
    return std::nullopt;
  }
  const auto year = static_cast<std::int64_t>(tm.tm_year) + 1900;
  const auto month = static_cast<unsigned>(tm.tm_mon + 1);
  const auto day = static_cast<unsigned>(tm.tm_mday);
  if (month < 1 || month > 12 || day < 1 || day > 31 ||
      tm.tm_hour < 0 || tm.tm_hour > 23 ||
      tm.tm_min < 0 || tm.tm_min > 59 ||
      tm.tm_sec < 0 || tm.tm_sec > 60) {
    return std::nullopt;
  }
  const auto seconds =
      cast_days_from_civil_(year, month, day) * 86'400LL +
      static_cast<std::int64_t>(tm.tm_hour) * 3'600LL +
      static_cast<std::int64_t>(tm.tm_min) * 60LL +
      static_cast<std::int64_t>(tm.tm_sec);
  if (seconds < std::numeric_limits<std::int64_t>::min() / 1'000'000'000LL ||
      seconds > std::numeric_limits<std::int64_t>::max() / 1'000'000'000LL) {
    return std::nullopt;
  }
  return seconds * 1'000'000'000LL;
}

std::optional<std::int64_t> cast_timeline_tick_from_utf8_(std::string_view text) {
  std::array<std::string_view, 1> views{text};
  std::array<std::int64_t, 1> shape{1};
  try {
    const auto parsed = pluss::codeh::ops_internal::timeline_from_utf8(
        std::span<const std::string_view>(views.data(), views.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));
    auto ticks = parsed.to_epoch_nanos_vector();
    if (ticks.empty() || !parsed.isvalid().all().item<bool>()) {
      return std::nullopt;
    }
    return ticks.front();
  } catch (...) {
    return std::nullopt;
  }
}

Thread cast_note_to_timeline_(
    const Thread& source,
    const ThreadCastOptions& options,
    std::string_view where) {
  std::vector<std::int64_t> ticks(static_cast<std::size_t>(source.size()), 0);
  std::vector<bool> valid(static_cast<std::size_t>(source.size()), true);
  for (std::int64_t i = 0; i < source.size(); ++i) {
    const auto pos = static_cast<std::size_t>(i);
    if (!source.is_valid(i)) {
      valid[pos] = false;
      continue;
    }
    const auto text = source.as_note().at(i);
    const auto tick = options.format.empty()
                          ? cast_timeline_tick_from_utf8_(text)
                          : cast_timeline_tick_from_pattern_(text, options.format);
    if (tick.has_value()) {
      ticks[pos] = *tick;
      continue;
    }
    if (options.invalid == ParseInvalid::Null) {
      valid[pos] = false;
      continue;
    }
    PLUSS_FAIL(std::string(where) + ": could not parse datetime value '" + std::string(text) + "'");
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(ticks.size())};
  auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(ticks.data(), ticks.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(out, bool_series_from_bools(valid));
}

} // namespace

Thread Thread::cast(ThreadCastTarget target, std::optional<pluss::codem::ScalarType> series_dtype) const {
  return cast(target, ThreadCastOptions{.series_dtype = series_dtype});
}

Thread Thread::cast(
    ThreadCastTarget target,
    const ThreadCastOptions& options) const {
  const auto series_dtype = options.series_dtype;
  switch (target) {
    case ThreadCastTarget::Series: {
      if (kind() == ThreadKind::Series) {
        if (!series_dtype.has_value() || as_series().scalar_type() == *series_dtype) {
          return *this;
        }
        return Thread(as_series().to(*series_dtype), validity_mask());
      }

      if (kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex) {
        const auto ticks = (kind() == ThreadKind::Timeline)
                               ? as_timeline().to_epoch_nanos_vector()
                               : as_timeindex().timeline().to_epoch_nanos_vector();
        auto out = pluss::Series::empty({static_cast<std::int64_t>(ticks.size())}, pluss::codem::ScalarType::Int64);
        if (!ticks.empty()) {
          std::memcpy(out.raw_data(), ticks.data(), ticks.size() * sizeof(std::int64_t));
        }
        if (series_dtype.has_value() && *series_dtype != pluss::codem::ScalarType::Int64) {
          out = out.to(*series_dtype);
        }
        return Thread(out, validity_mask());
      }

      PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::cast(Series): unsupported source type");
      const auto n = size();
      const auto dt = series_dtype.value_or(pluss::codem::ScalarType::Float64);

      if (dt == pluss::codem::ScalarType::Bool) {
        auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
        auto* p = reinterpret_cast<bool*>(out.raw_data());
        std::vector<bool> parsed_valid(static_cast<std::size_t>(n), true);
        for (std::int64_t i = 0; i < n; ++i) {
          if (!is_valid(i)) {
            parsed_valid[static_cast<std::size_t>(i)] = false;
            p[i] = false;
            continue;
          }
          bool value = false;
          if (!parse_bool_like(as_note().at(i), value)) {
            parsed_valid[static_cast<std::size_t>(i)] = false;
            p[i] = false;
            continue;
          }
          p[i] = value;
        }
        return Thread(out, optional_mask_from_valid(parsed_valid));
      }

      auto out = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
      auto* p = reinterpret_cast<double*>(out.raw_data());
      std::vector<bool> parsed_valid(static_cast<std::size_t>(n), true);
      for (std::int64_t i = 0; i < n; ++i) {
        if (!is_valid(i)) {
          parsed_valid[static_cast<std::size_t>(i)] = false;
          p[i] = 0.0;
          continue;
        }
        double value = 0.0;
        if (!parse_double_exact(as_note().at(i), value)) {
          parsed_valid[static_cast<std::size_t>(i)] = false;
          p[i] = 0.0;
          continue;
        }
        p[i] = value;
      }
      if (dt != pluss::codem::ScalarType::Float64) {
        out = out.to(dt);
      }
      return Thread(out, optional_mask_from_valid(parsed_valid));
    }
    case ThreadCastTarget::Note: {
      if (kind() == ThreadKind::Note) {
        return *this;
      }
      if (kind() == ThreadKind::Timeline) {
        return Thread(as_timeline().to_note_iso(), validity_mask());
      }
      if (kind() == ThreadKind::TimeIndex) {
        return Thread(as_timeindex().timeline().to_note_iso(), validity_mask());
      }

      PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::cast(Note): unsupported source type");
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(size()));
      for (std::int64_t i = 0; i < size(); ++i) {
        values.emplace_back(thread_scalar_string_at(*this, i));
      }
      return Thread(make_utf8_note(values), validity_mask());
    }
    case ThreadCastTarget::Timeline: {
      if (kind() == ThreadKind::Timeline) {
        return *this;
      }
      if (kind() == ThreadKind::TimeIndex) {
        return Thread(as_timeindex().timeline(), validity_mask());
      }
      if (kind() == ThreadKind::Series) {
        PLUSS_CHECK(is_numeric_series_type(as_series()),
                    "Thread::cast(Timeline): Series source must be numeric epoch-nanos or YYYYMMDD");
        const auto raw_f64 = as_series().to(pluss::codem::ScalarType::Float64);
        const auto platform = raw_f64.platform();

        pluss::Series active_mask =
            has_validity_mask()
                ? validity_mask()->to(platform)
                : pluss::Series::ones({size()}, pluss::codem::ScalarType::Bool).to(platform);

        const auto trunc = raw_f64.floor();
        const auto is_integral = raw_f64.eq(trunc);
        const auto in_yyyymmdd_range =
            raw_f64.ge(10000101.0).logical_and(raw_f64.le(99991231.0));
        const auto looks_like_yyyymmdd =
            is_integral.logical_and(in_yyyymmdd_range);
        const auto non_date_valid_rows =
            active_mask.logical_and(looks_like_yyyymmdd.logical_not());

        if (!non_date_valid_rows.any().item<bool>()) {
          const auto year = trunc.div(10000.0).floor().to(pluss::codem::ScalarType::Int64);
          const auto yyyymm = trunc.div(100.0).floor();
          const auto month =
              yyyymm.sub(year.to(pluss::codem::ScalarType::Float64).mul(100.0))
                  .to(pluss::codem::ScalarType::Int64);
          const auto day =
              trunc.sub(yyyymm.mul(100.0)).to(pluss::codem::ScalarType::Int64);

          auto out = pluss::Timeline::from_ymd(year, month, day);
          const auto invalid_calendar_rows =
              active_mask.logical_and(out.isvalid().logical_not());
          PLUSS_CHECK(
              !invalid_calendar_rows.any().item<bool>(),
              "Thread::cast(Timeline): invalid YYYYMMDD value(s)");
          return Thread(std::move(out), validity_mask());
        }

        std::vector<std::int64_t> ticks(static_cast<std::size_t>(size()), 0);
        for (std::int64_t i = 0; i < size(); ++i) {
          double v = 0.0;
          const bool ok = extract_numeric_value(as_series(), i, v);
          PLUSS_CHECK(ok, "Thread::cast(Timeline): failed numeric extraction from Series");
          ticks[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(v);
        }
        const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(ticks.size())};
        auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
            std::span<const std::int64_t>(ticks.data(), ticks.size()),
            std::span<const std::int64_t>(shape.data(), shape.size()));
        return Thread(out, validity_mask());
      }

      PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::cast(Timeline): unsupported source type");
      if (!options.format.empty() || options.invalid == ParseInvalid::Null) {
        return cast_note_to_timeline_(*this, options, "Thread::cast(Timeline)");
      }
      std::vector<std::string> vals;
      vals.reserve(static_cast<std::size_t>(size()));
      for (std::int64_t i = 0; i < size(); ++i) {
        vals.emplace_back(as_note().at(i));
      }
      std::vector<std::string_view> views;
      views.reserve(vals.size());
      for (const auto& v : vals) {
        views.emplace_back(v);
      }
      const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
      auto out = pluss::codeh::ops_internal::timeline_from_utf8(
          std::span<const std::string_view>(views.data(), views.size()),
          std::span<const std::int64_t>(shape.data(), shape.size()));
      return Thread(out, validity_mask());
    }
    case ThreadCastTarget::TimeIndex: {
      if (kind() == ThreadKind::TimeIndex) {
        return *this;
      }
      auto t = cast(ThreadCastTarget::Timeline, options).as_timeline();
      return Thread(t.to_timeindex(false), validity_mask());
    }
    default:
      return *this;
  }
}

Thread Thread::astype(pluss::codem::ScalarType dtype) const {
  return cast(ThreadCastTarget::Series, dtype);
}

TimeSeriesAccessor Thread::ts() const {
  return TimeSeriesAccessor(*this);
}

TimeSeriesAccessor::TimeSeriesAccessor(Thread base) : base_(std::move(base)) {}

namespace {

std::int64_t ts_duration_nanos_(std::string_view raw, std::string_view where) {
  auto first = raw.begin();
  auto last = raw.end();
  while (first != last && std::isspace(static_cast<unsigned char>(*first))) {
    ++first;
  }
  while (last != first && std::isspace(static_cast<unsigned char>(*(last - 1)))) {
    --last;
  }
  PLUSS_CHECK(first != last, std::string(where) + ": duration cannot be empty");

  std::int64_t value = 0;
  bool saw_digit = false;
  while (first != last && std::isdigit(static_cast<unsigned char>(*first))) {
    saw_digit = true;
    const auto digit = static_cast<std::int64_t>(*first - '0');
    PLUSS_CHECK(value <= (std::numeric_limits<std::int64_t>::max() - digit) / 10,
                std::string(where) + ": duration is too large");
    value = value * 10 + digit;
    ++first;
  }
  PLUSS_CHECK(saw_digit && value > 0, std::string(where) + ": duration must start with a positive integer");
  while (first != last && std::isspace(static_cast<unsigned char>(*first))) {
    ++first;
  }
  std::string unit(first, last);
  for (auto& ch : unit) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  PLUSS_CHECK(!unit.empty(), std::string(where) + ": duration unit is required, e.g. \"7D\", \"12H\", \"30min\"");

  std::int64_t multiplier = 0;
  if (unit == "ns" || unit == "nano" || unit == "nanos") {
    multiplier = 1;
  } else if (unit == "us" || unit == "micro" || unit == "micros") {
    multiplier = 1'000;
  } else if (unit == "ms" || unit == "milli" || unit == "millis") {
    multiplier = 1'000'000;
  } else if (unit == "s" || unit == "sec" || unit == "secs") {
    multiplier = 1'000'000'000;
  } else if (unit == "m" || unit == "min" || unit == "mins") {
    multiplier = 60LL * 1'000'000'000;
  } else if (unit == "h" || unit == "hr" || unit == "hrs") {
    multiplier = 60LL * 60LL * 1'000'000'000;
  } else if (unit == "d" || unit == "day" || unit == "days") {
    multiplier = 24LL * 60LL * 60LL * 1'000'000'000;
  } else if (unit == "w" || unit == "week" || unit == "weeks") {
    multiplier = 7LL * 24LL * 60LL * 60LL * 1'000'000'000;
  } else {
    PLUSS_FAIL(std::string(where) + ": unsupported duration unit '" + unit + "'");
  }
  PLUSS_CHECK(value <= std::numeric_limits<std::int64_t>::max() / multiplier,
              std::string(where) + ": duration is too large");
  return value * multiplier;
}

TimeWindowStatistic to_time_window_stat_(WindowStatistic statistic, std::string_view where) {
  switch (statistic) {
    case WindowStatistic::Sum:
      return TimeWindowStatistic::Sum;
    case WindowStatistic::Mean:
      return TimeWindowStatistic::Mean;
    case WindowStatistic::Var:
      return TimeWindowStatistic::Var;
    case WindowStatistic::Std:
      return TimeWindowStatistic::Std;
    default:
      PLUSS_FAIL(std::string(where) + ": supported time-window statistics are Sum, Mean, Var, and Std");
  }
}

pluss::TimeIndex ts_axis_to_index_(const Thread& axis, std::string_view where) {
  PLUSS_CHECK(axis.defined(), std::string(where) + ": time axis is undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": time axis must be Timeline or TimeIndex");
  return axis.kind() == ThreadKind::TimeIndex ? axis.as_timeindex()
                                              : axis.as_timeline().to_timeindex(false);
}

pluss::TimeIndex::AsofDirection ts_asof_direction_(TsAlignMode mode) {
  switch (mode) {
    case TsAlignMode::Backward:
      return pluss::TimeIndex::AsofDirection::Backward;
    case TsAlignMode::Forward:
      return pluss::TimeIndex::AsofDirection::Forward;
    case TsAlignMode::Nearest:
      return pluss::TimeIndex::AsofDirection::Nearest;
    case TsAlignMode::Exact:
    default:
      return pluss::TimeIndex::AsofDirection::Backward;
  }
}

void ts_require_numeric_thread_(const Thread& value, std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": values are undefined");
  PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
              std::string(where) + ": values must be numeric Series");
}

Thread ts_align_right_to_left_(const Thread& left,
                               const Thread& right,
                               const Thread& left_axis,
                               const Thread& right_axis,
                               TsAlignMode mode,
                               std::string_view where) {
  ts_require_numeric_thread_(left, where);
  ts_require_numeric_thread_(right, where);
  const auto left_ti = ts_axis_to_index_(left_axis, where);
  const auto right_ti = ts_axis_to_index_(right_axis, where);
  const auto n_left = left_ti.size();
  const auto n_right = right_ti.size();
  PLUSS_CHECK(left.size() == n_left,
              std::string(where) + ": left values and left time axis length mismatch");
  PLUSS_CHECK(right.size() == n_right,
              std::string(where) + ": right values and right time axis length mismatch");

  if (mode == TsAlignMode::Exact) {
    if (n_right == 0) {
      const auto platform = right.as_series().platform();
      return Thread(pluss::full({n_left},
                                std::numeric_limits<double>::quiet_NaN(),
                                pluss::Config{}.with_dtype(pluss::codem::ScalarType::Float64).with_platform(platform)),
                    pluss::Series::zeros({n_left}, pluss::codem::ScalarType::Bool, platform))
          .caselbl(left_axis);
    }

    const auto left_ticks = left_ti.timeline().epoch_nanos().to_vec<std::int64_t>();
    const auto right_ticks = right_ti.timeline().epoch_nanos().to_vec<std::int64_t>();
    std::unordered_map<std::int64_t, std::vector<std::int64_t>> positions;
    positions.reserve(right_ticks.size());
    for (std::int64_t row = 0; row < n_right; ++row) {
      const auto tick = right_ticks[static_cast<std::size_t>(row)];
      if (tick != pluss::Timeline::kNaT) {
        positions[tick].push_back(row);
      }
    }

    std::unordered_map<std::int64_t, std::size_t> seen;
    seen.reserve(left_ticks.size());
    std::vector<std::int64_t> rows;
    std::vector<bool> present;
    rows.reserve(left_ticks.size());
    present.reserve(left_ticks.size());
    for (const auto tick : left_ticks) {
      const auto occurrence = seen[tick]++;
      const auto found = positions.find(tick);
      if (tick != pluss::Timeline::kNaT && found != positions.end() && occurrence < found->second.size()) {
        const auto row = found->second[occurrence];
        rows.push_back(row);
        present.push_back(right.is_valid(row));
      } else {
        rows.push_back(0);
        present.push_back(false);
      }
    }

    auto out = right.take(std::span<const std::int64_t>(rows.data(), rows.size()));
    auto valid = bool_series_from_bools(present);
    const auto platform = right.as_series().platform();
    if (valid.platform() != platform) {
      valid = valid.to(platform);
    }
    if (detail::has_validity_mask(out)) {
      auto prior = *detail::validity_mask(out);
      if (prior.platform() != platform) {
        prior = prior.to(platform);
      }
      valid = valid.logical_and(prior);
    }
    auto values = out.as_series().to(pluss::codem::ScalarType::Float64);
    return Thread(values.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN()),
                  valid)
        .caselbl(left_axis);
  }

  pluss::Series idx;
  if (n_right == 0) {
    idx = pluss::full({n_left}, -1.0, pluss::codem::ScalarType::Int64);
  } else {
    idx = left_ti.join_asof(right_ti, ts_asof_direction_(mode), nullptr, true);
  }

  auto valid = idx.ge(0.0);
  if (n_left == 0 || n_right == 0) {
    const auto platform = right.as_series().platform();
    return Thread(pluss::full({n_left},
                              std::numeric_limits<double>::quiet_NaN(),
                              pluss::Config{}.with_dtype(pluss::codem::ScalarType::Float64).with_platform(platform)),
                  valid.to(platform));
  }

  const auto platform = right.as_series().platform();
  const auto safe_idx = idx.to(pluss::codem::ScalarType::Float64)
                            .clamp(0.0, static_cast<double>(n_right - 1))
                            .to(pluss::codem::ScalarType::Int64)
                            .to(platform);
  const auto gathered = right.as_series().to(pluss::codem::ScalarType::Float64).index_select(0, safe_idx);
  auto gathered_valid = right.notnull().as_series().to(platform).index_select(0, safe_idx);
  auto out_valid = valid.to(platform).logical_and(gathered_valid);
  auto out = Thread(gathered.masked_fill(out_valid.logical_not(), std::numeric_limits<double>::quiet_NaN()),
                    out_valid);
  return out.caselbl(left_axis);
}

} // namespace

const pluss::Series& TimeSeriesAccessor::series_payload_(const char* op) const {
  PLUSS_CHECK(base_.defined(), std::string("TimeSeriesAccessor::") + op + ": source Thread is undefined");
  PLUSS_CHECK(base_.kind() == ThreadKind::Series, std::string("TimeSeriesAccessor::") + op + ": source must be Series");
  PLUSS_CHECK(is_numeric_series_type(base_.as_series()),
              std::string("TimeSeriesAccessor::") + op + ": source Series must be numeric");
  return base_.as_series();
}

Thread TimeSeriesAccessor::returns(std::int64_t periods) const {
  return Thread(pluss::subsp::stats::ts::returns(series_payload_("returns"), periods));
}

Thread TimeSeriesAccessor::log_returns(std::int64_t periods) const {
  return Thread(pluss::subsp::stats::ts::log_returns(series_payload_("log_returns"), periods));
}

Thread TimeSeriesAccessor::diff(std::int64_t periods) const {
  return Thread(pluss::subsp::stats::ts::diff(series_payload_("diff"), periods));
}

Thread TimeSeriesAccessor::lag(std::int64_t periods) const {
  return Thread(pluss::subsp::stats::ts::lag(series_payload_("lag"), periods));
}

Thread TimeSeriesAccessor::lead(std::int64_t periods) const {
  return Thread(pluss::subsp::stats::ts::lead(series_payload_("lead"), periods));
}

Thread TimeSeriesAccessor::rolling(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  (void)series_payload_("rolling");
  return base_.moving(window, statistic, options);
}

Thread TimeSeriesAccessor::rolling(
    const Thread& axis,
    std::string_view duration,
    WindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  (void)series_payload_("rolling");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              "TimeSeriesAccessor::rolling: axis must be Timeline or TimeIndex");
  return base_.timed(axis,
                     ts_duration_nanos_(duration, "TimeSeriesAccessor::rolling"),
                     to_time_window_stat_(statistic, "TimeSeriesAccessor::rolling"),
                     options,
                     closure);
}

Thread TimeSeriesAccessor::timed(
    const Thread& axis,
    std::int64_t duration_nanos,
    TimeWindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  (void)series_payload_("timed");
  return base_.timed(axis, duration_nanos, statistic, options, closure);
}

Thread TimeSeriesAccessor::grouped(
    const Thread& groups,
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  (void)series_payload_("grouped");
  return base_.grouped(groups, window, statistic, options);
}

Thread TimeSeriesAccessor::expanding(
    WindowStatistic statistic,
    const StatisticOptions& options) const {
  (void)series_payload_("expanding");
  return base_.growing(statistic, options);
}

Thread TimeSeriesAccessor::ewm(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  (void)series_payload_("ewm");
  return base_.decay(alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls);
}

Thread TimeSeriesAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods, double epsilon) const {
  return Thread(pluss::subsp::stats::ts::zscore_rolling(
      series_payload_("zscore_rolling"), window, min_periods, epsilon));
}

Thread TimeSeriesAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode, std::optional<double> constant) const {
  return Thread(pluss::subsp::stats::ts::fill_gaps(series_payload_("fill_gaps"), mode, constant));
}

Thread TimeSeriesAccessor::align_to(const Thread& source_axis,
                                    const Thread& target_axis,
                                    TsAlignMode mode) const {
  const auto& source = series_payload_("align_to");
  const auto target_ti = ts_axis_to_index_(target_axis, "TimeSeriesAccessor::align_to");
  const auto platform = source.platform();
  auto target_values = Thread(
      pluss::Series::zeros({target_ti.size()}, pluss::codem::ScalarType::Float64, platform));
  return ts_align_right_to_left_(
      target_values, base_, target_axis, source_axis, mode, "TimeSeriesAccessor::align_to");
}

Thread TimeSeriesAccessor::align_to(const Thread& source_axis,
                                    const Thread& target_axis,
                                    TsAlignMode mode,
                                    pluss::subsp::stats::ts::GapFillMode fill_mode,
                                    std::optional<double> constant) const {
  return align_to(source_axis, target_axis, mode).ts().fill_gaps(fill_mode, constant);
}

DataFrame TimeSeriesAccessor::lag_block(pluss::msharp::RangeView::Int lags) const {
  PLUSS_CHECK(lags.size() > 0, "TimeSeriesAccessor::lag_block: expected at least one lag");
  const auto& series = series_payload_("lag_block");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(lags.size());
  for (const auto lag : lags) {
    out.emplace_back("lag" + std::to_string(lag), Thread(pluss::subsp::stats::ts::lag(series, lag)));
  }
  return DataFrame(std::move(out), base_.caselbl());
}

DataFrame TimeSeriesAccessor::rolling_block(pluss::msharp::RangeView::Int windows,
                                            const TsRollingBlockOptions& options) const {
  PLUSS_CHECK(windows.size() > 0, "TimeSeriesAccessor::rolling_block: expected at least one window");
  const auto& series = series_payload_("rolling_block");
  std::vector<std::pair<std::string, Thread>> out;
  for (const auto w : windows) {
    if (options.include_mean) {
      out.emplace_back("roll_mean_w" + std::to_string(w),
                       Thread(pluss::subsp::stats::ts::rolling_mean(series, w, options.min_periods)));
    }
    if (options.include_std) {
      out.emplace_back("roll_std_w" + std::to_string(w),
                       Thread(pluss::subsp::stats::ts::rolling_std(series, w, options.min_periods)));
    }
    if (options.include_min) {
      out.emplace_back("roll_min_w" + std::to_string(w),
                       Thread(pluss::subsp::stats::ts::rolling_min(series, w, options.min_periods)));
    }
    if (options.include_max) {
      out.emplace_back("roll_max_w" + std::to_string(w),
                       Thread(pluss::subsp::stats::ts::rolling_max(series, w, options.min_periods)));
    }
    if (options.include_sum) {
      out.emplace_back("roll_sum_w" + std::to_string(w),
                       Thread(pluss::subsp::stats::ts::rolling_sum(series, w, options.min_periods)));
    }
    if (options.include_var) {
      out.emplace_back("roll_var_w" + std::to_string(w),
                       Thread(pluss::subsp::stats::ts::rolling_var(series, w, options.min_periods)));
    }
  }
  return DataFrame(std::move(out), base_.caselbl());
}

namespace {

std::string_view time_series_resample_op_key_(pluss::subsp::stats::ts::ResampleOp op) {
  using ResampleOp = pluss::subsp::stats::ts::ResampleOp;
  switch (op) {
    case ResampleOp::Mean:
      return "mean";
    case ResampleOp::Sum:
      return "sum";
    case ResampleOp::Min:
      return "min";
    case ResampleOp::Max:
      return "max";
    case ResampleOp::First:
      return "first";
    case ResampleOp::Last:
      return "last";
    case ResampleOp::Count:
      return "count";
  }
  PLUSS_FAIL("TimeSeriesAccessor::resample: unsupported ResampleOp");
}

}  // namespace

TimeSeriesResample TimeSeriesAccessor::resample(const Thread& axis,
                                                std::string_view freq,
                                                pluss::subsp::stats::ts::ResampleOp op,
                                                std::int64_t step) const {
  return resample(axis,
                  pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step},
                  op);
}

TimeSeriesResample TimeSeriesAccessor::resample(
    const Thread& axis,
    const pluss::TimeIndex::ResampleOptions& options,
    pluss::subsp::stats::ts::ResampleOp op) const {
  PLUSS_CHECK(axis.defined(), "TimeSeriesAccessor::resample: axis Thread is undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              "TimeSeriesAccessor::resample: axis must be Timeline or TimeIndex");
  const auto time_index = (axis.kind() == ThreadKind::TimeIndex)
                              ? axis.as_timeindex()
                              : axis.as_timeline().to_timeindex(false);
  const auto bins = time_index.resample_bins(options);
  const auto values =
      op == pluss::subsp::stats::ts::ResampleOp::Count
          ? time_index.resample_sum(base_.notnull().as_series().to(pluss::codem::ScalarType::Int64), options)
          : time_index.resample_agg(
                series_payload_("resample"),
                options,
                time_series_resample_op_key_(op));
  return TimeSeriesResample{
      .labels = Thread(bins.labels),
      .values = Thread(values),
  };
}

Thread TimeSeriesAccessor::add(const Thread& other,
                               const Thread& left_axis,
                               const Thread& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("add");
  return base_.add(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::add"));
}

Thread TimeSeriesAccessor::sub(const Thread& other,
                               const Thread& left_axis,
                               const Thread& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("sub");
  return base_.sub(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::sub"));
}

Thread TimeSeriesAccessor::mul(const Thread& other,
                               const Thread& left_axis,
                               const Thread& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("mul");
  return base_.mul(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::mul"));
}

Thread TimeSeriesAccessor::div(const Thread& other,
                               const Thread& left_axis,
                               const Thread& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("div");
  return base_.div(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::div"));
}

Thread TimeSeriesAccessor::eq(const Thread& other,
                              const Thread& left_axis,
                              const Thread& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("eq");
  return base_.eq(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::eq"));
}

Thread TimeSeriesAccessor::ne(const Thread& other,
                              const Thread& left_axis,
                              const Thread& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("ne");
  return base_.ne(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::ne"));
}

Thread TimeSeriesAccessor::lt(const Thread& other,
                              const Thread& left_axis,
                              const Thread& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("lt");
  return base_.lt(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::lt"));
}

Thread TimeSeriesAccessor::le(const Thread& other,
                              const Thread& left_axis,
                              const Thread& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("le");
  return base_.le(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::le"));
}

Thread TimeSeriesAccessor::gt(const Thread& other,
                              const Thread& left_axis,
                              const Thread& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("gt");
  return base_.gt(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::gt"));
}

Thread TimeSeriesAccessor::ge(const Thread& other,
                              const Thread& left_axis,
                              const Thread& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("ge");
  return base_.ge(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::ge"));
}
