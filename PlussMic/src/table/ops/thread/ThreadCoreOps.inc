Thread::Thread(pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl) : impl_(std::move(impl)) {}

Thread::Thread() = default;

Thread::Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const Thread& other) = default;
Thread::Thread(Thread&& other) noexcept = default;
Thread& Thread::operator=(const Thread& other) = default;
Thread& Thread::operator=(Thread&& other) noexcept = default;
Thread::~Thread() = default;

bool Thread::defined() const noexcept { return static_cast<bool>(impl_); }
bool Thread::unique() const noexcept { return impl_.unique(); }

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

std::int64_t Thread::size() const { return impl_ref_().size_; }
ThreadKind Thread::kind() const { return impl_ref_().kind_; }
std::string Thread::dtype_name() const { return impl_ref_().dtype_name_; }
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
  return Thread(build_impl(src.payload_, src.validity_mask_, make_utf8_note(values), src.display_name_));
}

Thread Thread::clear_display_labels() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, src.validity_mask_, std::nullopt, src.display_name_));
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
  return Thread(build_impl(src.payload_, src.validity_mask_, src.display_labels_, std::move(name)));
}

Thread Thread::clear_display_name() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, src.validity_mask_, src.display_labels_, std::nullopt));
}

Thread Thread::with_validity_mask(const pluss::Series& validity_mask) const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, validity_mask, src.display_labels_, src.display_name_));
}

Thread& Thread::with_validity_mask_(const pluss::Series& validity_mask) {
  auto& w = mut_impl_();
  validate_validity_mask(validity_mask, w.size_);
  w.validity_mask_ = validity_mask;
  return *this;
}

Thread Thread::clear_validity_mask() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, std::nullopt, src.display_labels_, src.display_name_));
}

Thread& Thread::clear_validity_mask_() {
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  return *this;
}

Thread Thread::isnull() const {
  const auto& src = impl_ref_();
  if (!src.validity_mask_.has_value()) {
    return Thread(bool_series_zeros(src.size_));
  }
  return Thread(src.validity_mask_->logical_not());
}

Thread Thread::notnull() const {
  if (!has_validity_mask()) {
    auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(out.raw_data());
    for (std::int64_t i = 0; i < size(); ++i) {
      p[i] = true;
    }
    return Thread(out);
  }
  return Thread(*validity_mask());
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
  if (!has_validity_mask()) {
    return *this;
  }
  auto out = as_series().where(*validity_mask(), value);
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

Thread Thread::abs() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::abs is only valid for Series payload");
  return Thread(as_series().abs(), validity_mask());
}

Thread Thread::clip(double min_value, double max_value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::clip is only valid for Series payload");
  return Thread(as_series().clamp(min_value, max_value), validity_mask());
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
  *s = s->where(*w.validity_mask_, value);
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
    out_mask = src.validity_mask_->take(idx_series);
  }

  std::optional<pluss::Note> out_display_labels = std::nullopt;
  if (src.display_labels_.has_value()) {
    out_display_labels = make_utf8_note(note_take_strings(*src.display_labels_, indices, src.size_));
  }

  return Thread(build_impl(std::move(out_payload), std::move(out_mask), std::move(out_display_labels), src.display_name_));
}

Thread Thread::filter(const pluss::Series& bool_mask) const {
  const auto indices = true_indices_from_mask(bool_mask, size());
  return take(indices);
}

Thread Thread::dropna() const {
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

Thread Thread::ffill() const {
  return forward_fill_thread(*this);
}

Thread Thread::bfill() const {
  return backward_fill_thread(*this);
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

Thread Thread::distinct(bool dropna) const {
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
      if (!dropna && !null_emitted) {
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

DataFrameView Thread::tally(std::int64_t top_n, bool dropna, bool render) const {
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
    if (!is_valid && dropna) {
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

ThreadView Thread::describe(bool render) const {
  const auto name = has_display_name() ? display_name() : std::string("value");

  if (kind() == ThreadKind::Series && is_numeric_series_type(as_series())) {
    auto desc_frame = DataFrame({{name, *this}}).describe(false);
    auto out = desc_frame.features(name);
    return ThreadView(std::move(out), render, "describe");
  }

  if (kind() == ThreadKind::Note) {
    const auto non_null = count(true).item<std::int64_t>();
    const auto distinct_count = distinct(/*dropna=*/true).count(true).item<std::int64_t>();
    auto counts = tally(/*top_n=*/0, /*dropna=*/true, /*render=*/false);

    std::string top_value = "NA";
    std::int64_t top_freq = 0;
    if (counts.ncases() > 0) {
      top_value = counts.cases(0, "value");
      top_freq = std::stoll(counts.cases(0, "count"));
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
                   .with_display_labels(Thread(make_utf8_note({"count", "unique", "top", "freq"})))
                   .with_display_name(name);
    return ThreadView(std::move(out), render, "describe");
  }

  PLUSS_FAIL("Thread::describe supports numeric Series and Note payloads");
}

pluss::Series Thread::sum() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::sum is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::sum on an empty (all-null) selection");
  return s.sum();
}

pluss::Series Thread::count(bool count_true) const {
  if (kind() == ThreadKind::Series) {
    auto s = as_series();
    if (has_validity_mask()) {
      s = s.masked_select(notnull().as_series());
    }
    return s.count(count_true);
  }

  PLUSS_CHECK(count_true, "Thread::count(false) is only valid for Bool Series payload");
  std::int64_t total = size();
  if (has_validity_mask()) {
    const auto valid = notnull().as_series().to_vec<bool>();
    total = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  }
  auto out = pluss::Series::empty({1}, pluss::codem::ScalarType::Int64);
  reinterpret_cast<std::int64_t*>(out.raw_data())[0] = total;
  return out;
}

pluss::Series Thread::any(bool skipna) const {
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
  return bool_series_from_bools({true_count > 0});
}

pluss::Series Thread::all(bool skipna) const {
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
  return bool_series_from_bools({true_count == s.numel()});
}

pluss::Series Thread::mean() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::mean is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::mean on an empty (all-null) selection");
  return s.mean();
}

pluss::Series Thread::median() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::median is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::median on an empty (all-null) selection");
  return s.median();
}

pluss::Series Thread::min() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::min is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::min on an empty (all-null) selection");
  return s.min();
}

pluss::Series Thread::max() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::max is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::max on an empty (all-null) selection");
  return s.max();
}

pluss::Series Thread::var(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::var is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::var on an empty (all-null) selection");
  return s.var(unbiased);
}

pluss::Series Thread::std(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::std is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::std on an empty (all-null) selection");
  return s.std(unbiased);
}

double Thread::corr(const Thread& other, ThreadCorrMethod method) const {
  PLUSS_CHECK(kind() == ThreadKind::Series && other.kind() == ThreadKind::Series,
              "Thread::corr: both operands must be Series");
  PLUSS_CHECK(size() == other.size(), "Thread::corr: size mismatch");
  PLUSS_CHECK(is_numeric_series_type(as_series()) && is_numeric_series_type(other.as_series()),
              "Thread::corr: both operands must be numeric Series");

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

Thread Thread::replace(std::string_view from, std::string_view to) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::replace is only valid for Note payload");
  return Thread(as_note().replace(from, to), validity_mask());
}

pluss::Series Thread::contains(std::string_view needle) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::contains is only valid for Note payload");
  return bool_series_from_u8(as_note().contains(needle));
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
  return Thread(apply_math_series_scalar(as_series(), other, op, reverse), validity_mask());
}

Thread Thread::numeric_binary(const Thread& other, ThreadBinaryMathOp op) const {
  if (kind() != ThreadKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  PLUSS_CHECK(other.kind() == ThreadKind::Series && is_numeric_series_type(other.as_series()),
              "Thread::numeric_binary: rhs must be numeric Series");
  return Thread(
      apply_math_series_series(as_series(), other.as_series(), op),
      combine_validity_masks(*this, other));
}

Thread Thread::compare(double other, ThreadBinaryCompareOp op) const {
  if (kind() == ThreadKind::Series) {
    return Thread(apply_compare_series_scalar(as_series(), other, op), validity_mask());
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
  return Thread(out, validity_mask());
}

Thread Thread::compare(const Thread& other, ThreadBinaryCompareOp op) const {
  PLUSS_CHECK(size() == other.size(), "Thread::compare: size mismatch");

  if (kind() == ThreadKind::Series && other.kind() == ThreadKind::Series) {
    return Thread(
        apply_compare_series_series(as_series(), other.as_series(), op),
        combine_validity_masks(*this, other));
  }

  if (kind() == ThreadKind::Timeline && other.kind() == ThreadKind::Timeline) {
    const auto& lhs = as_timeline();
    const auto& rhs = other.as_timeline();
    pluss::Series out = lhs.eq(rhs);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        out = lhs.eq(rhs);
        break;
      case ThreadBinaryCompareOp::Ne:
        out = lhs.ne(rhs);
        break;
      case ThreadBinaryCompareOp::Lt:
        out = lhs.lt(rhs);
        break;
      case ThreadBinaryCompareOp::Le:
        out = lhs.le(rhs);
        break;
      case ThreadBinaryCompareOp::Gt:
        out = lhs.gt(rhs);
        break;
      case ThreadBinaryCompareOp::Ge:
        out = lhs.ge(rhs);
        break;
    }
    return Thread(out, combine_validity_masks(*this, other));
  }

  if (kind() == ThreadKind::TimeIndex && other.kind() == ThreadKind::TimeIndex) {
    const auto& lhs = as_timeindex().timeline();
    const auto& rhs = other.as_timeindex().timeline();
    pluss::Series out = lhs.eq(rhs);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        out = lhs.eq(rhs);
        break;
      case ThreadBinaryCompareOp::Ne:
        out = lhs.ne(rhs);
        break;
      case ThreadBinaryCompareOp::Lt:
        out = lhs.lt(rhs);
        break;
      case ThreadBinaryCompareOp::Le:
        out = lhs.le(rhs);
        break;
      case ThreadBinaryCompareOp::Gt:
        out = lhs.gt(rhs);
        break;
      case ThreadBinaryCompareOp::Ge:
        out = lhs.ge(rhs);
        break;
    }
    return Thread(out, combine_validity_masks(*this, other));
  }

  if (kind() == ThreadKind::Note && other.kind() == ThreadKind::Note) {
    const auto& lhs = as_note();
    const auto& rhs = other.as_note();
    std::vector<std::uint8_t> raw;
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        raw = (lhs == rhs);
        break;
      case ThreadBinaryCompareOp::Ne:
        raw = (lhs != rhs);
        break;
      case ThreadBinaryCompareOp::Lt:
        raw = (lhs < rhs);
        break;
      case ThreadBinaryCompareOp::Le:
        raw = (lhs <= rhs);
        break;
      case ThreadBinaryCompareOp::Gt:
        raw = (lhs > rhs);
        break;
      case ThreadBinaryCompareOp::Ge:
        raw = (lhs >= rhs);
        break;
    }
    return Thread(bool_series_from_u8(raw), combine_validity_masks(*this, other));
  }

  if ((kind() == ThreadKind::Timeline && other.kind() == ThreadKind::TimeIndex)
      || (kind() == ThreadKind::TimeIndex && other.kind() == ThreadKind::Timeline)) {
    const auto lhs_t = (kind() == ThreadKind::Timeline) ? as_timeline() : as_timeindex().timeline();
    const auto rhs_t = (other.kind() == ThreadKind::Timeline) ? other.as_timeline() : other.as_timeindex().timeline();
    pluss::Series out = lhs_t.eq(rhs_t);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        out = lhs_t.eq(rhs_t);
        break;
      case ThreadBinaryCompareOp::Ne:
        out = lhs_t.ne(rhs_t);
        break;
      case ThreadBinaryCompareOp::Lt:
        out = lhs_t.lt(rhs_t);
        break;
      case ThreadBinaryCompareOp::Le:
        out = lhs_t.le(rhs_t);
        break;
      case ThreadBinaryCompareOp::Gt:
        out = lhs_t.gt(rhs_t);
        break;
      case ThreadBinaryCompareOp::Ge:
        out = lhs_t.ge(rhs_t);
        break;
    }
    return Thread(out, combine_validity_masks(*this, other));
  }

  PLUSS_FAIL("Thread::compare: unsupported payload combination");
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
  PLUSS_CHECK(size() == lo.size() && size() == hi.size(),
              "Thread::btw(Thread,Thread,...): lower/upper bounds must match thread size");
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

Thread Thread::cast(
    ThreadCastTarget target,
    std::optional<pluss::codem::ScalarType> series_dtype) const {
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
      auto t = cast(ThreadCastTarget::Timeline).as_timeline();
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

Thread TimeSeriesAccessor::rolling_mean(std::int64_t window, std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::rolling_mean(series_payload_("rolling_mean"), window, min_periods));
}

Thread TimeSeriesAccessor::rolling_std(std::int64_t window, std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::rolling_std(series_payload_("rolling_std"), window, min_periods));
}

Thread TimeSeriesAccessor::rolling_sum(std::int64_t window, std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::rolling_sum(series_payload_("rolling_sum"), window, min_periods));
}

Thread TimeSeriesAccessor::rolling_var(std::int64_t window, std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::rolling_var(series_payload_("rolling_var"), window, min_periods));
}

Thread TimeSeriesAccessor::rolling_min(std::int64_t window, std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::rolling_min(series_payload_("rolling_min"), window, min_periods));
}

Thread TimeSeriesAccessor::rolling_max(std::int64_t window, std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::rolling_max(series_payload_("rolling_max"), window, min_periods));
}

Thread TimeSeriesAccessor::ema(double alpha) const {
  return Thread(pluss::subsp::stats::ts::ema(series_payload_("ema"), alpha));
}

Thread TimeSeriesAccessor::ewm_mean(double alpha) const {
  return Thread(pluss::subsp::stats::ts::ewm_mean(series_payload_("ewm_mean"), alpha));
}

Thread TimeSeriesAccessor::ewm_std(double alpha, double epsilon) const {
  return Thread(pluss::subsp::stats::ts::ewm_std(series_payload_("ewm_std"), alpha, epsilon));
}

Thread TimeSeriesAccessor::expanding_mean(std::int64_t min_periods) const {
  return Thread(pluss::subsp::stats::ts::expanding_mean(series_payload_("expanding_mean"), min_periods));
}

Thread TimeSeriesAccessor::expanding_std(std::int64_t min_periods, double epsilon) const {
  return Thread(pluss::subsp::stats::ts::expanding_std(series_payload_("expanding_std"), min_periods, epsilon));
}

Thread TimeSeriesAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods, double epsilon) const {
  return Thread(pluss::subsp::stats::ts::zscore_rolling(
      series_payload_("zscore_rolling"), window, min_periods, epsilon));
}

Thread TimeSeriesAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode, std::optional<double> constant) const {
  return Thread(pluss::subsp::stats::ts::fill_gaps(series_payload_("fill_gaps"), mode, constant));
}

TimeSeriesResample TimeSeriesAccessor::resample(const Thread& axis,
                                                std::string_view freq,
                                                pluss::subsp::stats::ts::ResampleOp op,
                                                std::int64_t step) const {
  PLUSS_CHECK(axis.defined(), "TimeSeriesAccessor::resample: axis Thread is undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              "TimeSeriesAccessor::resample: axis must be Timeline or TimeIndex");
  const auto time_index = (axis.kind() == ThreadKind::TimeIndex)
                              ? axis.as_timeindex()
                              : axis.as_timeline().to_timeindex(false);
  const auto out = pluss::subsp::stats::ts::resample(
      time_index, series_payload_("resample"), freq, op, step);
  PLUSS_CHECK(out.labels.has_value(), "TimeSeriesAccessor::resample: missing resample labels");
  return TimeSeriesResample{
      .labels = Thread(*out.labels),
      .values = Thread(out.values),
  };
}
