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
const Thread& Thread::values() const noexcept { return *this; }
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

Thread Thread::with_validity_mask(const pluss::Series& validity_mask) const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, validity_mask));
}

Thread& Thread::with_validity_mask_(const pluss::Series& validity_mask) {
  auto& w = mut_impl_();
  validate_validity_mask(validity_mask, w.size_);
  w.validity_mask_ = validity_mask;
  return *this;
}

Thread Thread::clear_validity_mask() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, std::nullopt));
}

Thread& Thread::clear_validity_mask_() {
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  return *this;
}

pluss::Series Thread::isnull() const {
  const auto& src = impl_ref_();
  if (!src.validity_mask_.has_value()) {
    return bool_series_zeros(src.size_);
  }
  return src.validity_mask_->logical_not();
}

pluss::Series Thread::notnull() const {
  if (!has_validity_mask()) {
    auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(out.raw_data());
    for (std::int64_t i = 0; i < size(); ++i) {
      p[i] = true;
    }
    return out;
  }
  return *validity_mask();
}

pluss::Series Thread::isnan() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isnan is only valid for Series payload");
  return as_series().isnan();
}

pluss::Series Thread::isfinite() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isfinite is only valid for Series payload");
  auto finite = as_series().isfinite();
  if (has_validity_mask()) {
    finite = finite.logical_and(*validity_mask());
  }
  return finite;
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

const pluss::Series& Thread::series() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::series is only valid for Series payload");
  return as_series();
}

const pluss::Note& Thread::note() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::note is only valid for Note payload");
  return as_note();
}

const pluss::Timeline& Thread::timeline() const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex,
              "Thread::timeline is only valid for Timeline/TimeIndex payload");
  return kind() == ThreadKind::Timeline ? as_timeline() : as_timeindex().timeline();
}

const pluss::TimeIndex& Thread::timeindex() const {
  PLUSS_CHECK(kind() == ThreadKind::TimeIndex, "Thread::timeindex is only valid for TimeIndex payload");
  return as_timeindex();
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

Thread Thread::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(0, clipped);
}

Thread Thread::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(size() - clipped, size());
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

  return Thread(build_impl(std::move(out_payload), std::move(out_mask)));
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

Thread Thread::ffill() const {
  return forward_fill_thread(*this);
}

Thread Thread::bfill() const {
  return backward_fill_thread(*this);
}

pluss::Series Thread::sum() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::sum is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::sum on an empty (all-null) selection");
  return s.sum();
}

pluss::Series Thread::mean() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::mean is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::mean on an empty (all-null) selection");
  return s.mean();
}

pluss::Series Thread::min() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::min is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::min on an empty (all-null) selection");
  return s.min();
}

pluss::Series Thread::max() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::max is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::max on an empty (all-null) selection");
  return s.max();
}

pluss::Series Thread::var(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::var is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
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
    s = s.masked_select(*validity_mask());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::std on an empty (all-null) selection");
  return s.std(unbiased);
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
                    "Thread::cast(Timeline): Series source must be numeric epoch-nanos");
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
