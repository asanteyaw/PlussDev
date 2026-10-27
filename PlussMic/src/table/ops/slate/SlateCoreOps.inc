Slate::Slate(pluss::msharp::intrusive_ptr<detail::SlateImpl> impl) : impl_(std::move(impl)) {}

Slate::Slate() = default;

namespace {

std::optional<Slate> range_display_labels_from_options_(std::int64_t size,
                                                         const SlateOptions& options) {
  if (!options.rangelbl) {
    return std::nullopt;
  }
  return default_slate_cases_(size);
}

pluss::msharp::intrusive_ptr<detail::SlateImpl> build_slate_impl_with_options_(
    Payload payload,
    std::optional<pluss::Series> validity_mask,
    const SlateOptions& options) {
  const auto size = std::visit([](const auto& value) { return infer_size(value); }, payload);
  return build_impl(std::move(payload),
                    std::move(validity_mask),
                    range_display_labels_from_options_(size, options),
                    std::nullopt,
                    options.rangelbl ? LabelKind::Case : LabelKind::Default);
}

} // namespace

Slate::Slate(const pluss::Series& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Slate::Slate(const pluss::Series& data, const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Slate::Slate(const pluss::Series& data,
               std::optional<pluss::Series> validity_mask,
               const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Slate::Slate(const pluss::Note& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Slate::Slate(const pluss::Note& data, const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Slate::Slate(const pluss::Note& data,
               std::optional<pluss::Series> validity_mask,
               const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Slate::Slate(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Slate::Slate(const pluss::Timeline& data, const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Slate::Slate(const pluss::Timeline& data,
               std::optional<pluss::Series> validity_mask,
               const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Slate::Slate(const pluss::Duration& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Slate::Slate(const pluss::Duration& data, const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::nullopt, options)) {}

Slate::Slate(const pluss::Duration& data,
               std::optional<pluss::Series> validity_mask,
               const SlateOptions& options)
    : impl_(build_slate_impl_with_options_(Payload{data}, std::move(validity_mask), options)) {}

Slate::Slate(const Slate& other) = default;
Slate::Slate(Slate&& other) noexcept = default;
Slate& Slate::operator=(const Slate& other) = default;
Slate& Slate::operator=(Slate&& other) noexcept = default;
Slate::~Slate() = default;

bool Slate::defined() const noexcept { return static_cast<bool>(impl_); }
bool Slate::unique() const noexcept { return impl_.unique(); }
bool Slate::same_payload_(const Slate& other) const noexcept {
  return impl_.get() == other.impl_.get();
}

const detail::SlateImpl& Slate::impl_ref_() const {
  PLUSS_CHECK(defined(), "Slate: undefined handle");
  return *impl_;
}

detail::SlateImpl& Slate::mut_impl_() {
  PLUSS_CHECK(defined(), "Slate: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

namespace detail {

LabelKind label_kind(const Slate& slate) {
  return slate.impl_ref_().label_kind_;
}

Slate with_label_kind(const Slate& slate, LabelKind kind) {
  auto out = slate;
  return with_label_kind_(out, kind);
}

Slate& with_label_kind_(Slate& slate, LabelKind kind) {
  slate.mut_impl_().label_kind_ = kind;
  return slate;
}

} // namespace detail

std::int64_t Slate::size() const { return impl_ref_().size_; }
std::int64_t Slate::ncases() const { return size(); }
std::pair<std::int64_t, std::int64_t> Slate::shape() const {
  return {size(), 1};
}
SlateKind Slate::kind() const { return impl_ref_().kind_; }
std::string Slate::ptype() const {
  switch (kind()) {
    case SlateKind::Series:
      return "Series";
    case SlateKind::Note:
      return "Note";
    case SlateKind::Timeline:
      return "Timeline";
    case SlateKind::Duration:
      return "Duration";
  }
  return "Unknown";
}
std::string Slate::dtype() const { return impl_ref_().dtype_; }
SlateValue Slate::value() const { return SlateValue(*this); }

pluss::kindle::Stream Slate::kindelize() const {
  return pluss::kindle::Stream::from_slate(*this);
}

pluss::scribble::Slot Slate::scribbelize() const {
  return kindelize().scribbelize();
}
bool Slate::is_valid(std::int64_t row) const {
  const auto idx = normalize_index(row, size());
  if (!has_validity_mask()) {
    return true;
  }
  const auto& mask = *validity_mask();
  if (mask.platform() == pluss::codem::Platform::CPU &&
      mask.scalar_type() == pluss::codem::ScalarType::Bool &&
      mask.is_contiguous()) {
    return reinterpret_cast<const bool*>(mask.raw_data())[static_cast<std::size_t>(idx)];
  }
  return mask.host<bool>().array().data.at(static_cast<std::size_t>(idx));
}

bool Slate::has_validity_mask() const { return impl_ref_().validity_mask_.has_value(); }

const std::optional<pluss::Series>& Slate::validity_mask() const {
  return impl_ref_().validity_mask_;
}

bool Slate::has_display_labels() const {
  return impl_ref_().display_labels_.has_value();
}

Slate Slate::display_labels() const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.display_labels_.has_value(), "Slate::display_labels: labels are not set");
  return *src.display_labels_;
}

Slate Slate::with_display_labels(const Slate& labels) const {
  PLUSS_CHECK(labels.defined(), "Slate::with_display_labels: labels are undefined");
  PLUSS_CHECK(labels.size() == size(), "Slate::with_display_labels: labels length mismatch");
  auto clean_labels = labels.label().reset();

  const auto& src = impl_ref_();
  return Slate(build_impl(src.payload_,
                           src.validity_mask_,
                           std::move(clean_labels),
                           src.display_name_,
                           LabelKind::Custom));
}

Slate& Slate::with_display_labels_(const Slate& labels) {
  PLUSS_CHECK(labels.defined(), "Slate::with_display_labels_: labels are undefined");
  PLUSS_CHECK(labels.size() == size(), "Slate::with_display_labels_: labels length mismatch");

  auto& w = mut_impl_();
  w.display_labels_ = labels.label().reset();
  w.label_kind_ = LabelKind::Custom;
  return *this;
}

Slate Slate::clear_display_labels() const {
  const auto& src = impl_ref_();
  return Slate(build_impl(src.payload_, src.validity_mask_, std::nullopt, src.display_name_));
}

Slate& Slate::clear_display_labels_() {
  mut_impl_().display_labels_.reset();
  mut_impl_().label_kind_ = LabelKind::Default;
  return *this;
}

bool Slate::has_caselbl() const {
  return has_display_labels();
}

Slate Slate::caselbl() const {
  if (has_display_labels()) {
    return display_labels();
  }
  return default_slate_cases_(size());
}

Slate Slate::caselbl(const Slate& labels) const {
  return with_display_labels(labels);
}

Slate& Slate::caselbl_(const Slate& labels) {
  return with_display_labels_(labels);
}

Slate Slate::rangelbl() const {
  auto out = with_display_labels(default_slate_cases_(size()));
  return detail::with_label_kind(out, LabelKind::Case);
}

Slate& Slate::rangelbl_() {
  with_display_labels_(default_slate_cases_(size()));
  detail::with_label_kind_(*this, LabelKind::Case);
  return *this;
}

Slate Slate::clear_caselbl() const {
  return clear_display_labels();
}

Slate& Slate::clear_caselbl_() {
  return clear_display_labels_();
}

SlateLabelAccessor Slate::label() const {
  return SlateLabelAccessor(*this);
}

SlateLabelAccessor::SlateLabelAccessor(Slate base)
    : base_(std::move(base)) {}

Slate SlateLabelAccessor::values() const {
  return base_.caselbl();
}

bool SlateLabelAccessor::has() const {
  return base_.has_caselbl();
}

Slate SlateLabelAccessor::set(const Slate& labels) const {
  return base_.caselbl(labels);
}

Slate SlateLabelAccessor::range() const {
  return base_.rangelbl();
}

Slate SlateLabelAccessor::reset() const {
  return base_.clear_caselbl();
}

Slate SlateLabelAccessor::align(const Slate& target_labels, const AlignOptions& options) const {
  return base_.align(target_labels, options);
}

namespace {

std::string label_key_at_(const Slate& labels, std::int64_t row) {
  return detail::valid_at(labels, row) ? slate_scalar_string_at(labels, row) : std::string{"<NULL>"};
}

Slate make_position_slate_(const std::vector<std::int64_t>& positions, const Slate* labels = nullptr) {
  auto out = Slate(pluss::make_series(positions));
  if (labels != nullptr && labels->defined() && labels->size() == out.size()) {
    out = out.label().set(*labels);
  }
  return out;
}

Slate make_label_note_slate_(const std::vector<std::string>& values, std::string_view display_name) {
  return Slate(pluss::make_note(values)).label().reset().name(std::string(display_name));
}

struct LabelSetOperands_ {
  Slate left{};
  Slate right{};
  bool dtype_preserving{false};
};

bool same_payload_platform_(const Slate& left, const Slate& right) {
  return slate_payload_platform_(left) == slate_payload_platform_(right);
}

Slate series_label_as_(const Slate& value,
                        pluss::codem::ScalarType dtype,
                        pluss::codem::Platform platform) {
  auto series = value.as_series();
  if (series.scalar_type() != dtype) {
    series = series.to(dtype);
  }
  if (series.platform() != platform) {
    series = series.to(platform);
  }
  return Slate(series, detail::validity_mask(value)).label().reset();
}

Slate timeline_payload_(const Slate& value) {
  if (value.kind() == SlateKind::Timeline) {
    return value;
  }
  PLUSS_FAIL("Slate::label: expected temporal payload");
}

LabelSetOperands_ label_set_operands_(const Slate& left, const Slate& right) {
  if (left.kind() == right.kind()) {
    if (left.kind() == SlateKind::Series) {
      const auto platform = left.as_series().platform();
      if (left.as_series().scalar_type() == right.as_series().scalar_type()) {
        const auto dtype = left.as_series().scalar_type();
        return {series_label_as_(left, dtype, platform), series_label_as_(right, dtype, platform), true};
      }
      if (is_numeric_series_type(left.as_series()) && is_numeric_series_type(right.as_series())) {
        return {
            series_label_as_(left, pluss::codem::ScalarType::Float64, platform),
            series_label_as_(right, pluss::codem::ScalarType::Float64, platform),
            true};
      }
      return {left, right, false};
    }
    if (!same_payload_platform_(left, right)) {
      return {left, right, false};
    }
    return {left.label().reset(), right.label().reset(), true};
  }

  const auto left_temporal = left.kind() == SlateKind::Timeline;
  const auto right_temporal = right.kind() == SlateKind::Timeline;
  if (left_temporal && right_temporal && same_payload_platform_(timeline_payload_(left), timeline_payload_(right))) {
    return {timeline_payload_(left).label().reset(), timeline_payload_(right).label().reset(), true};
  }

  if (left.kind() == SlateKind::Series && right.kind() == SlateKind::Series &&
      is_numeric_series_type(left.as_series()) && is_numeric_series_type(right.as_series())) {
    const auto platform = left.as_series().platform();
    return {
        series_label_as_(left, pluss::codem::ScalarType::Float64, platform),
        series_label_as_(right, pluss::codem::ScalarType::Float64, platform),
        true};
  }

  return {left, right, false};
}

Slate label_take_reset_(const Slate& labels, const std::vector<std::int64_t>& indices) {
  return labels.take(std::span<const std::int64_t>(indices.data(), indices.size())).label().reset();
}

Slate label_concat_reset_(const Slate& left,
                           const std::vector<std::int64_t>& left_indices,
                           const Slate& right,
                           const std::vector<std::int64_t>& right_indices,
                           std::string_view display_name) {
  auto left_part = label_take_reset_(left, left_indices);
  auto right_part = label_take_reset_(right, right_indices);
  if (left_part.size() == 0) {
    return right_part.name(std::string(display_name));
  }
  if (right_part.size() == 0) {
    return left_part.name(std::string(display_name));
  }
  return left_part.append(right_part).label().reset().name(std::string(display_name));
}

std::unordered_set<std::string> label_key_set_(const Slate& labels) {
  std::unordered_set<std::string> out;
  out.reserve(static_cast<std::size_t>(labels.size()));
  for (std::int64_t row = 0; row < labels.size(); ++row) {
    out.insert(label_key_at_(labels, row));
  }
  return out;
}

std::unordered_map<std::string, std::vector<std::int64_t>> label_positions_by_key_(const Slate& labels) {
  std::unordered_map<std::string, std::vector<std::int64_t>> out;
  out.reserve(static_cast<std::size_t>(labels.size()));
  for (std::int64_t row = 0; row < labels.size(); ++row) {
    out[label_key_at_(labels, row)].push_back(row);
  }
  return out;
}

} // namespace

Slate SlateLabelAccessor::unite(const Slate& other) const {
  const auto raw_left = values();
  const auto operands = label_set_operands_(raw_left, other);
  const auto& left = operands.left;
  const auto& right = operands.right;
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(left.size() + right.size()));
  std::vector<std::string> fallback;
  fallback.reserve(static_cast<std::size_t>(left.size() + right.size()));
  std::vector<std::int64_t> left_indices;
  std::vector<std::int64_t> right_indices;
  left_indices.reserve(static_cast<std::size_t>(left.size()));
  right_indices.reserve(static_cast<std::size_t>(right.size()));
  for (std::int64_t row = 0; row < left.size(); ++row) {
    auto key = label_key_at_(left, row);
    if (seen.insert(key).second) {
      if (operands.dtype_preserving) {
        left_indices.push_back(row);
      } else {
        fallback.push_back(std::move(key));
      }
    }
  }
  for (std::int64_t row = 0; row < right.size(); ++row) {
    auto key = label_key_at_(right, row);
    if (seen.insert(key).second) {
      if (operands.dtype_preserving) {
        right_indices.push_back(row);
      } else {
        fallback.push_back(std::move(key));
      }
    }
  }
  if (!operands.dtype_preserving) {
    return make_label_note_slate_(fallback, "unite");
  }
  return label_concat_reset_(left, left_indices, right, right_indices, "unite");
}

Slate SlateLabelAccessor::intersect(const Slate& other) const {
  const auto raw_left = values();
  const auto operands = label_set_operands_(raw_left, other);
  const auto& left = operands.left;
  const auto& right = operands.right;
  const auto right_keys = label_key_set_(right);
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(left.size()));
  std::vector<std::string> fallback;
  std::vector<std::int64_t> left_indices;
  for (std::int64_t row = 0; row < left.size(); ++row) {
    auto key = label_key_at_(left, row);
    if (right_keys.contains(key) && seen.insert(key).second) {
      if (operands.dtype_preserving) {
        left_indices.push_back(row);
      } else {
        fallback.push_back(std::move(key));
      }
    }
  }
  if (!operands.dtype_preserving) {
    return make_label_note_slate_(fallback, "intersect");
  }
  return label_take_reset_(left, left_indices).name("intersect");
}

Slate SlateLabelAccessor::diff(const Slate& other) const {
  const auto raw_left = values();
  const auto operands = label_set_operands_(raw_left, other);
  const auto& left = operands.left;
  const auto& right = operands.right;
  const auto right_keys = label_key_set_(right);
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(left.size()));
  std::vector<std::string> fallback;
  std::vector<std::int64_t> left_indices;
  for (std::int64_t row = 0; row < left.size(); ++row) {
    auto key = label_key_at_(left, row);
    if (!right_keys.contains(key) && seen.insert(key).second) {
      if (operands.dtype_preserving) {
        left_indices.push_back(row);
      } else {
        fallback.push_back(std::move(key));
      }
    }
  }
  if (!operands.dtype_preserving) {
    return make_label_note_slate_(fallback, "diff");
  }
  return label_take_reset_(left, left_indices).name("diff");
}

Slate SlateLabelAccessor::symdiff(const Slate& other) const {
  const auto raw_left = values();
  const auto operands = label_set_operands_(raw_left, other);
  const auto& left = operands.left;
  const auto& right = operands.right;
  const auto left_keys = label_key_set_(left);
  const auto right_keys = label_key_set_(right);
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(left.size() + right.size()));
  std::vector<std::string> fallback;
  std::vector<std::int64_t> left_indices;
  std::vector<std::int64_t> right_indices;
  for (std::int64_t row = 0; row < left.size(); ++row) {
    auto key = label_key_at_(left, row);
    if (!right_keys.contains(key) && seen.insert(key).second) {
      if (operands.dtype_preserving) {
        left_indices.push_back(row);
      } else {
        fallback.push_back(std::move(key));
      }
    }
  }
  for (std::int64_t row = 0; row < right.size(); ++row) {
    auto key = label_key_at_(right, row);
    if (!left_keys.contains(key) && seen.insert(key).second) {
      if (operands.dtype_preserving) {
        right_indices.push_back(row);
      } else {
        fallback.push_back(std::move(key));
      }
    }
  }
  if (!operands.dtype_preserving) {
    return make_label_note_slate_(fallback, "symdiff");
  }
  return label_concat_reset_(left, left_indices, right, right_indices, "symdiff");
}

Slate SlateLabelAccessor::find(const Slate& targets) const {
  const auto labels = values();
  const auto& target_labels = targets;
  const auto positions_by_key = label_positions_by_key_(labels);
  std::vector<std::int64_t> positions;
  positions.reserve(static_cast<std::size_t>(target_labels.size()));
  for (std::int64_t row = 0; row < target_labels.size(); ++row) {
    const auto key = label_key_at_(target_labels, row);
    const auto it = positions_by_key.find(key);
    positions.push_back(it == positions_by_key.end() || it->second.empty() ? -1 : it->second.front());
  }
  return make_position_slate_(positions, &target_labels).name("find");
}

Slate SlateLabelAccessor::pos(double value) const {
  const auto singleton = Slate(pluss::make_series({value}));
  return pos(label_key_at_(singleton, 0));
}

Slate SlateLabelAccessor::pos(std::string_view value) const {
  const auto labels = values();
  std::vector<std::int64_t> positions;
  for (std::int64_t row = 0; row < labels.size(); ++row) {
    if (label_key_at_(labels, row) == value) {
      positions.push_back(row);
    }
  }
  PLUSS_CHECK(!positions.empty(), "Slate::label().pos: label not found");
  return make_position_slate_(positions).name("pos");
}

Slate SlateLabelAccessor::slice(std::string_view start, std::string_view stop) const {
  const auto labels = values();
  std::optional<std::int64_t> begin;
  std::optional<std::int64_t> end;
  for (std::int64_t row = 0; row < labels.size(); ++row) {
    const auto key = label_key_at_(labels, row);
    if (!begin.has_value() && key == start) {
      begin = row;
    }
    if (key == stop) {
      end = row;
    }
  }
  PLUSS_CHECK(begin.has_value(), "Slate::label().slice: start label not found");
  PLUSS_CHECK(end.has_value(), "Slate::label().slice: stop label not found");
  PLUSS_CHECK(*begin <= *end, "Slate::label().slice: start label occurs after stop label");
  return labels.slice(*begin, *end + 1).name("slice");
}

Slate SlateLabelAccessor::fillnull(double value) const {
  return base_.caselbl(base_.caselbl().fillnull(value));
}

Slate SlateLabelAccessor::fillnull(std::string_view value) const {
  return base_.caselbl(base_.caselbl().fillnull(value));
}

Slate SlateLabelAccessor::fillnull(const pluss::Timeline& value) const {
  return base_.caselbl(base_.caselbl().fillnull(value));
}

Slate SlateLabelAccessor::fillnull(const pluss::Duration& value) const {
  return base_.caselbl(base_.caselbl().fillnull(value));
}

Slate SlateLabelAccessor::duplicated(bool keep_first, bool keep_last) const {
  const auto labels = values();
  return labels.duplicated(keep_first, keep_last).label().set(labels).name("duplicated");
}

Slate SlateLabelAccessor::repeated() const {
  const auto labels = values();
  return labels.repeated().label().set(labels).name("repeated");
}

Slate SlateLabelAccessor::distinct(bool dropnull) const {
  return values().distinct(dropnull).name("distinct");
}

Slate SlateLabelAccessor::freq(std::int64_t top_n, bool dropnull) const {
  return detail::freq_count_slate(values(), top_n, dropnull, "freq");
}

Slate SlateLabelAccessor::has(const Slate& values) const {
  const auto labels = this->values();
  return labels.has(values).label().set(labels).name("has");
}

Slate SlateLabelAccessor::isnull() const {
  const auto labels = values();
  return labels.isnull().label().set(labels).name("isnull");
}

Slate SlateLabelAccessor::notnull() const {
  const auto labels = values();
  return labels.notnull().label().set(labels).name("notnull");
}

Slate SlateLabelAccessor::map(const std::function<SlateValue(const SlateValue&)>& fn) const {
  return base_.caselbl(values().map(fn));
}

Slate SlateLabelAccessor::take(pluss::msharp::RangeView::Int indices) const {
  std::vector<std::int64_t> owned;
  owned.reserve(indices.size());
  for (const auto idx : indices) {
    owned.push_back(idx);
  }
  return values().take(std::span<const std::int64_t>(owned.data(), owned.size()));
}

Slate SlateLabelAccessor::slice(std::int64_t start, std::int64_t stop) const {
  return values().slice(start, stop);
}

Slate SlateLabelAccessor::sort(bool ascending, bool nulls_last) const {
  const auto labels = values();
  std::vector<std::int64_t> positions(static_cast<std::size_t>(labels.size()));
  std::iota(positions.begin(), positions.end(), static_cast<std::int64_t>(0));
  const auto sorted = labels.label().set(Slate(pluss::make_series(positions))).arrange(ascending, nulls_last);
  return base_.take(sorted.label().values().as_series().host<std::int64_t>());
}

bool SlateLabelAccessor::monotonic(bool ascending, bool strict, bool nulls_last) const {
  const auto labels = values();
  for (std::int64_t row = 1; row < labels.size(); ++row) {
    const auto cmp = detail::compare_rows(labels, row - 1, row, nulls_last);
    if (ascending) {
      if (strict ? cmp >= 0 : cmp > 0) {
        return false;
      }
    } else if (strict ? cmp <= 0 : cmp < 0) {
      return false;
    }
  }
  return true;
}

Slate Slate::align(const Slate& target_labels, const AlignOptions& options) const {
  return align_slate_to_label_values_(*this, target_labels, options, "Slate::align");
}

bool Slate::named() const {
  return impl_ref_().display_name_.has_value();
}

std::string Slate::name() const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(src.display_name_.has_value(), "Slate::name: name is not set");
  return *src.display_name_;
}

Slate Slate::name(std::string value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!value.empty(), "Slate::name(value): name must not be empty");
  return Slate(build_impl(src.payload_,
                           src.validity_mask_,
                           src.display_labels_,
                           std::move(value),
                           src.label_kind_));
}

Slate Slate::unname() const {
  const auto& src = impl_ref_();
  return Slate(build_impl(src.payload_,
                           src.validity_mask_,
                           src.display_labels_,
                           std::nullopt,
                           src.label_kind_));
}

Slate Slate::with_validity_mask(const pluss::Series& validity_mask) const {
  const auto& src = impl_ref_();
  return Slate(build_impl(src.payload_,
                           validity_mask,
                           src.display_labels_,
                           src.display_name_,
                           src.label_kind_));
}

Slate& Slate::with_validity_mask_(const pluss::Series& validity_mask) {
  auto& w = mut_impl_();
  validate_validity_mask(validity_mask, w.size_);
  w.validity_mask_ = validity_mask;
  return *this;
}

Slate Slate::clear_validity_mask() const {
  const auto& src = impl_ref_();
  return Slate(build_impl(src.payload_,
                           std::nullopt,
                           src.display_labels_,
                           src.display_name_,
                           src.label_kind_));
}

Slate& Slate::clear_validity_mask_() {
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  return *this;
}

Slate Slate::isnull() const {
  auto out = Slate(slate_non_missing_mask(*this).logical_not());
  return detail::has_display_labels(*this)
             ? detail::with_display_labels(out, detail::display_labels(*this))
             : out;
}

Slate Slate::notnull() const {
  auto out = Slate(slate_non_missing_mask(*this));
  return detail::has_display_labels(*this)
             ? detail::with_display_labels(out, detail::display_labels(*this))
             : out;
}

Slate Slate::isnan() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::isnan is only valid for Series payload");
  return Slate(as_series().isnan());
}

Slate Slate::isfinite() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::isfinite is only valid for Series payload");
  auto finite = as_series().isfinite();
  if (has_validity_mask()) {
    finite = finite.logical_and(notnull().as_series());
  }
  return Slate(finite);
}

Slate Slate::invalid() const {
  Slate out;
  if (kind() == SlateKind::Series) {
    auto invalid = as_series().isfinite().logical_not();
    invalid = invalid.logical_or(isnull().as_series().to(invalid.platform()));
    out = Slate(invalid);
  } else {
    out = isnull();
  }
  return detail::has_display_labels(*this)
             ? detail::with_display_labels(out, detail::display_labels(*this))
             : out;
}

Slate Slate::fillnull(double value) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::fillnull(double) is only valid for Series payload");
  const auto missing = notnull().as_series().logical_not();
  if (!has_validity_mask() && !series_uses_nan_missing(as_series())) {
    return *this;
  }
  auto out = as_series().where(missing, value);
  return Slate(out, std::nullopt);
}

Slate Slate::fillnull(std::string_view value) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::fillnull(string) is only valid for Note payload");
  if (!has_validity_mask()) {
    return *this;
  }
  const auto valid = validity_mask()->host<bool>();
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
  return Slate(rebuild_note_like(as_note(), values), std::nullopt);
}

Slate Slate::fillnull(const pluss::Timeline& value) const {
  PLUSS_CHECK(kind() == SlateKind::Timeline,
              "Slate::fillnull(Timeline) is only valid for Timeline payload");
  PLUSS_CHECK(value.defined(), "Slate::fillnull(Timeline): replacement value is undefined");
  PLUSS_CHECK(value.numel() == 1, "Slate::fillnull(Timeline): replacement value must be scalar (numel == 1)");
  if (!has_validity_mask()) {
    return *this;
  }

  return Slate(as_timeline().where(*validity_mask(), value), std::nullopt);
}

Slate Slate::fillnull(const pluss::Duration& value) const {
  PLUSS_CHECK(kind() == SlateKind::Duration,
              "Slate::fillnull(Duration) is only valid for Duration payload");
  PLUSS_CHECK(value.defined() && value.numel() == 1,
              "Slate::fillnull(Duration): replacement must contain exactly one value");
  const auto replacement_nanos = value.host().front();
  PLUSS_CHECK(replacement_nanos != pluss::Duration::kNaT,
              "Slate::fillnull(Duration): replacement cannot be NaT");
  const auto replacement = pluss::duration::nanos(replacement_nanos);
  auto filled = as_duration().fill_nat(replacement);
  if (has_validity_mask()) {
    filled = filled.where(*validity_mask(), replacement);
  }
  return Slate(std::move(filled), std::nullopt);
}

namespace {

pluss::Series slate_put_mask_(const Slate& bool_mask,
                               std::int64_t expected_size,
                               std::string_view where) {
  PLUSS_CHECK(bool_mask.defined(), std::string(where) + ": mask is undefined");
  PLUSS_CHECK(bool_mask.kind() == SlateKind::Series, std::string(where) + ": mask must be a Series slate");
  const auto& mask = bool_mask.as_series();
  (void)bool_vec_from_series(mask, expected_size, where);
  return mask;
}

void validate_slate_put_value_(const Slate& base,
                                const Slate& value,
                                std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": replacement value is undefined");
  PLUSS_CHECK(value.kind() == base.kind(),
              std::string(where) + ": replacement kind must match target slate kind");
  PLUSS_CHECK(value.size() == 1 || value.size() == base.size(),
              std::string(where) + ": replacement value must be scalar-like or match target length");
}

std::optional<pluss::Series> nullify_matches_mask(const Slate& t,
                                                  const pluss::Series& matches,
                                                  bool& changed);

} // namespace

Slate Slate::put(const Slate& bool_mask, const Slate& value) const {
  const auto aligned_mask = align_slate_to_cases_(bool_mask, *this, "Slate::put(mask)");
  const auto aligned_value = align_slate_to_cases_(value, *this, "Slate::put(value)", true);
  const auto mask = slate_put_mask_(aligned_mask, size(), "Slate::put");
  validate_slate_put_value_(*this, aligned_value, "Slate::put");

  if (kind() == SlateKind::Note) {
    const auto hit = mask.host<bool>();
    auto out_valid = notnull().as_series().host<bool>();
    std::vector<std::string> out_values;
    out_values.reserve(static_cast<std::size_t>(size()));
    const bool scalar_value = aligned_value.size() == 1;
    for (std::int64_t i = 0; i < size(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
      if (hit[idx]) {
        const auto rhs_row = scalar_value ? static_cast<std::int64_t>(0) : i;
        out_values.emplace_back(aligned_value.as_note().at(rhs_row));
        out_valid[idx] = detail::valid_at(aligned_value, rhs_row);
      } else {
        out_values.emplace_back(as_note().at(i));
      }
    }
    return Slate(rebuild_note_like(as_note(), out_values), optional_mask_from_valid(out_valid));
  }

  return select({aligned_mask}, {aligned_value}, *this);
}

Slate Slate::put(const Slate& bool_mask, double value) const {
  if (kind() == SlateKind::Series) {
    const auto& s = as_series();
    return put(bool_mask, Slate(pluss::Series::full({1}, value, s.scalar_type(), s.platform())));
  }
  return put(bool_mask, Slate(pluss::make_series(std::vector<double>{value}, {1})));
}

Slate Slate::put(const Slate& bool_mask, std::string_view value) const {
  return put(bool_mask, Slate(pluss::make_note(std::vector<std::string>{std::string(value)})));
}

Slate Slate::put(const Slate& bool_mask, pluss::null_t) const {
  const auto aligned_mask = align_slate_to_cases_(bool_mask, *this, "Slate::put(mask)");
  const auto mask = slate_put_mask_(aligned_mask, size(), "Slate::put");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, mask, changed);
  if (!changed) {
    return *this;
  }

  switch (kind()) {
    case SlateKind::Series:
      return Slate(as_series(), new_mask);
    case SlateKind::Note:
      return Slate(as_note(), new_mask);
    case SlateKind::Timeline:
      return Slate(as_timeline(), new_mask);
    default:
      PLUSS_FAIL("Slate::put(null): unsupported payload kind");
  }
}

Slate Slate::select(pluss::msharp::RangeView::Of<Slate> conditions,
                      pluss::msharp::RangeView::Of<Slate> choices,
                      const Slate& default_value) {
  PLUSS_CHECK(!conditions.empty(), "Slate::select: at least one condition is required");
  PLUSS_CHECK(conditions.size() == choices.size(),
              "Slate::select: condition/choice count mismatch");
  PLUSS_CHECK(default_value.defined(), "Slate::select: default value is undefined");

  const auto out_kind = choices[0].kind();
  PLUSS_CHECK(default_value.kind() == out_kind,
              "Slate::select: default value kind must match choice kind");

  std::vector<pluss::Series> cond_masks;
  cond_masks.reserve(conditions.size());
  for (const auto& cond : conditions) {
    PLUSS_CHECK(cond.defined(), "Slate::select: condition is undefined");
    PLUSS_CHECK(cond.kind() == SlateKind::Series,
                "Slate::select: conditions must be Series-backed boolean masks");
    PLUSS_CHECK(cond.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
                "Slate::select: conditions must use Bool dtype");
    cond_masks.push_back(cond.as_series());
  }
  const auto output_size = cond_masks.front().numel();
  for (const auto& mask : cond_masks) {
    PLUSS_CHECK(mask.numel() == output_size,
                "Slate::select: all conditions must have the same length");
  }

  auto validity_series = [](const Slate& t) -> pluss::Series {
    if (t.has_validity_mask()) {
      return t.notnull().as_series();
    }
    return pluss::Series::ones({1}, pluss::codem::ScalarType::Bool);
  };

  std::vector<pluss::Series> choice_validity;
  choice_validity.reserve(choices.size());
  for (const auto& choice : choices) {
    PLUSS_CHECK(choice.defined(), "Slate::select: choice is undefined");
    PLUSS_CHECK(choice.kind() == out_kind,
                "Slate::select: all choices must use the same payload kind");
    PLUSS_CHECK(choice.size() == 1 || choice.size() == output_size,
                "Slate::select: each choice must be scalar-like or match condition length");
    choice_validity.push_back(validity_series(choice));
  }
  PLUSS_CHECK(default_value.size() == 1 || default_value.size() == output_size,
              "Slate::select: default value must be scalar-like or match condition length");

  auto expand_validity = [](const pluss::Series& valid, std::int64_t output_size) {
    if (valid.numel() == output_size) {
      return valid;
    }
    PLUSS_CHECK(valid.numel() == 1,
                "Slate::select: validity mask must match output size or be scalar-like");
    const bool keep = valid.host<bool>()[0];
    return pluss::Series::full(
        {output_size},
        keep ? 1.0 : 0.0,
        pluss::codem::ScalarType::Bool,
        valid.platform());
  };

  switch (out_kind) {
    case SlateKind::Series: {
      std::vector<pluss::Series> payloads;
      payloads.reserve(choices.size());
      for (const auto& choice : choices) {
        payloads.push_back(choice.as_series());
      }
      auto out = pluss::series::select(
          pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
          pluss::msharp::RangeView::Of<pluss::Series>(payloads),
          default_value.as_series());

      std::vector<pluss::Series> expanded_validity;
      expanded_validity.reserve(choice_validity.size());
      for (const auto& valid : choice_validity) {
        expanded_validity.push_back(expand_validity(valid, out.numel()));
      }
      const auto out_valid = optional_mask_from_valid(
          pluss::series::select(
              pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
              pluss::msharp::RangeView::Of<pluss::Series>(expanded_validity),
              expand_validity(validity_series(default_value), out.numel()))
              .host<bool>());
      return Slate(out, std::move(out_valid));
    }

    case SlateKind::Timeline: {
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
          pluss::series::select(
              pluss::msharp::RangeView::Of<pluss::Series>(cond_masks),
              pluss::msharp::RangeView::Of<pluss::Series>(expanded_validity),
              expand_validity(validity_series(default_value), out.numel()))
              .host<bool>());
      return Slate(out, std::move(out_valid));
    }

    case SlateKind::Note: {
      std::vector<std::string> out_values;
      out_values.reserve(static_cast<std::size_t>(output_size));
      auto out_valid = expand_validity(validity_series(default_value), output_size).host<bool>();
      const bool default_scalar = default_value.size() == 1;
      for (std::int64_t i = 0; i < output_size; ++i) {
        out_values.emplace_back(default_value.as_note().at(default_scalar ? 0 : i));
      }

      std::vector<std::vector<bool>> cond_hits;
      cond_hits.reserve(cond_masks.size());
      for (const auto& cond_mask : cond_masks) {
        cond_hits.push_back(cond_mask.host<bool>());
      }

      for (std::size_t cond_idx = conditions.size(); cond_idx > 0; --cond_idx) {
        const auto& choice = choices[cond_idx - 1];
        const bool scalar_choice = choice.size() == 1;
        const auto choice_valid = expand_validity(choice_validity[cond_idx - 1], output_size).host<bool>();
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

      return Slate(rebuild_note_like(default_value.as_note(), out_values),
                    optional_mask_from_valid(out_valid));
    }
    default:
      PLUSS_FAIL("Slate::select: unsupported payload kind");
  }
}

Slate Slate::select(pluss::msharp::RangeView::Of<Slate> conditions,
                      pluss::msharp::RangeView::Of<Slate> choices,
                      double default_value) {
  PLUSS_CHECK(!choices.empty(), "Slate::select(double): at least one choice is required");
  PLUSS_CHECK(choices[0].kind() == SlateKind::Series,
              "Slate::select(double): choices must be Series-backed");
  return select(
      conditions,
      choices,
      Slate(pluss::Series::full(
          {1},
          default_value,
          choices[0].as_series().scalar_type(),
          choices[0].as_series().platform())));
}

Slate Slate::select(pluss::msharp::RangeView::Of<Slate> conditions,
                      pluss::msharp::RangeView::Of<Slate> choices,
                      std::string_view default_value) {
  return select(
      conditions,
      choices,
      Slate(pluss::make_note(std::vector<std::string>{std::string(default_value)})));
}

Slate Slate::select(pluss::msharp::RangeView::Of<Slate> conditions,
                      pluss::msharp::RangeView::Of<Slate> choices,
                      const pluss::Timeline& default_value) {
  PLUSS_CHECK(default_value.defined(), "Slate::select(Timeline): default value is undefined");
  PLUSS_CHECK(default_value.numel() == 1, "Slate::select(Timeline): default value must be scalar-like");
  return select(
      conditions,
      choices,
      Slate(pluss::make_timeline(
          std::vector<std::string>{std::string(default_value.to_note_iso().at(0))})));
}

Slate Slate::select(pluss::msharp::RangeView::Of<Slate> conditions,
                      pluss::msharp::RangeView::Of<Slate> choices,
                      pluss::null_t) {
  PLUSS_CHECK(!choices.empty(), "Slate::select(null): at least one choice is required");
  const auto& seed = choices[0];
  if (seed.kind() == SlateKind::Note) {
    std::vector<std::string> empty_values(
        static_cast<std::size_t>(seed.size()), std::string{});
    auto empty = Slate(pluss::make_note(empty_values));
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

Slate Slate::abs() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::abs is only valid for Series payload");
  return Slate(as_series().abs(), validity_mask());
}

Slate Slate::clip(double min_value, double max_value) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::clip is only valid for Series payload");
  return Slate(as_series().clamp(min_value, max_value), validity_mask());
}

Slate Slate::round(std::int64_t decimals) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::round is only valid for Series payload");
  auto s = as_series();
  if (decimals == 0) {
    return Slate(s.round(), validity_mask());
  }
  const auto scale = std::pow(10.0, static_cast<double>(decimals));
  return Slate(s.mul(scale).round().div(scale), validity_mask());
}

Slate Slate::log() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::log is only valid for Series payload");
  auto s = as_series();
  if (s.scalar_type() != pluss::codem::ScalarType::Float16
      && s.scalar_type() != pluss::codem::ScalarType::BFloat16
      && s.scalar_type() != pluss::codem::ScalarType::Float32
      && s.scalar_type() != pluss::codem::ScalarType::Float64) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  return Slate(s.log(), validity_mask());
}

Slate Slate::exp() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::exp is only valid for Series payload");
  auto s = as_series();
  if (s.scalar_type() != pluss::codem::ScalarType::Float16
      && s.scalar_type() != pluss::codem::ScalarType::BFloat16
      && s.scalar_type() != pluss::codem::ScalarType::Float32
      && s.scalar_type() != pluss::codem::ScalarType::Float64) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  return Slate(s.exp(), validity_mask());
}

Slate& Slate::fillnull_(double value) {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::fillnull_(double) is only valid for Series payload");
  if (!has_validity_mask()) {
    return *this;
  }
  auto& w = mut_impl_();
  auto* s = std::get_if<pluss::Series>(&w.payload_);
  PLUSS_CHECK(s != nullptr, "Slate::fillnull_: internal payload mismatch");
  *s = s->where(w.validity_mask_->logical_not(), value);
  w.validity_mask_.reset();
  return *this;
}

Slate& Slate::fillnull_(std::string_view value) {
  *this = fillnull(value);
  return *this;
}

Slate& Slate::fillnull_(const pluss::Timeline& value) {
  *this = fillnull(value);
  return *this;
}

Slate& Slate::fillnull_(const pluss::Duration& value) {
  *this = fillnull(value);
  return *this;
}

SlateView Slate::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = slice(0, clipped);
  return SlateView(std::move(out), render, "head");
}

SlateView Slate::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = slice(size() - clipped, size());
  return SlateView(std::move(out), render, "tail");
}

SlateView Slate::view(std::int64_t start, std::int64_t stop, bool render) const {
  auto out = slice(start, stop);
  return SlateView(std::move(out), render, "view");
}

SlateView Slate::top(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = arrange(false, true).slice(0, clipped);
  return SlateView(std::move(out), render, "top");
}

SlateView Slate::bottom(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  auto out = arrange(true, true).slice(0, clipped);
  return SlateView(std::move(out), render, "bottom");
}

namespace {

std::optional<pluss::Series> append_validity_mask_(const Slate& left,
                                                   const Slate& right,
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

Slate append_payload_(const Slate& left, const Slate& right, std::string_view where) {
  PLUSS_CHECK(left.kind() == right.kind(),
              std::string(where) + ": payload kinds must match");
  switch (left.kind()) {
    case SlateKind::Series: {
      PLUSS_CHECK(left.as_series().scalar_type() == right.as_series().scalar_type(),
                  std::string(where) + ": Series dtypes must match");
      PLUSS_CHECK(left.as_series().platform() == right.as_series().platform(),
                  std::string(where) + ": Series platforms must match");
      std::vector<pluss::Series> tail{right.as_series()};
      auto values = left.as_series().unite(tail, 0);
      return Slate(values, append_validity_mask_(left, right, values.platform()));
    }
    case SlateKind::Note: {
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(left.size() + right.size()));
      for (const auto value : left.as_note().vector()) {
        values.emplace_back(value);
      }
      for (const auto value : right.as_note().vector()) {
        values.emplace_back(value);
      }
      return Slate(pluss::make_note(values),
                    append_validity_mask_(left, right, pluss::codem::Platform::CPU));
    }
    case SlateKind::Timeline: {
      PLUSS_CHECK(slate_payload_platform_(left) == slate_payload_platform_(right),
                  std::string(where) + ": Timeline platforms must match");
      auto values = left.as_timeline().concat(right.as_timeline());
      return Slate(values, append_validity_mask_(left, right, slate_payload_platform_(left)));
    }
    case SlateKind::Duration: {
      auto values = left.as_duration().host();
      const auto right_values = right.as_duration().host();
      values.insert(values.end(), right_values.begin(), right_values.end());
      return Slate(pluss::duration::nanos(values),
                    append_validity_mask_(left, right, pluss::codem::Platform::CPU));
    }
  }
  PLUSS_FAIL(std::string(where) + ": unsupported Slate kind");
}

std::vector<std::int64_t> slate_sample_indices_(std::int64_t total,
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

Slate Slate::append(const Slate& other) const {
  auto out = append_payload_(*this, other, "Slate::append");
  PLUSS_CHECK(has_caselbl() == other.has_caselbl(),
              "Slate::append: both Slates must either have case labels or both be unlabeled");
  if (has_caselbl()) {
    out = out.caselbl(append_payload_(caselbl(), other.caselbl(), "Slate::append(caselbl)"));
  }
  return out;
}

Slate Slate::sample(std::int64_t n) const {
  return sample(n, std::random_device{}(), false);
}

Slate Slate::sample(std::int64_t n, std::uint64_t random_state, bool replace) const {
  PLUSS_CHECK(n >= 0, "Slate::sample: n must be >= 0");
  const auto total = size();
  const auto draw = replace ? n : std::min<std::int64_t>(n, total);
  std::mt19937_64 rng(random_state);
  return take(slate_sample_indices_(total, draw, replace, rng));
}

Slate Slate::sample_fraction_(double frac,
                                std::optional<std::uint64_t> random_state,
                                bool replace) const {
  PLUSS_CHECK(std::isfinite(frac), "Slate::sample: frac must be finite");
  PLUSS_CHECK(frac >= 0.0, "Slate::sample: frac must be >= 0");
  PLUSS_CHECK(replace || frac <= 1.0, "Slate::sample: frac > 1 requires replace = true");
  const auto draw = static_cast<std::int64_t>(
      std::llround(frac * static_cast<double>(size())));
  return sample(draw, random_state.value_or(std::random_device{}()), replace);
}

Slate Slate::shuffle(std::optional<std::uint64_t> random_state) const {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(size()));
  std::iota(idx.begin(), idx.end(), std::int64_t{0});
  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));
  std::shuffle(idx.begin(), idx.end(), rng);
  return take(idx);
}

Slate Slate::shift(std::int64_t periods) const {
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
    src_valid = validity_mask()->host<bool>();
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

Slate Slate::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, size());
  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    indices.push_back(i);
  }
  return take(indices);
}

Slate Slate::take(std::span<const std::int64_t> indices) const {
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
      return duration_take(payload, indices, src.size_);
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

  std::optional<Slate> out_display_labels = std::nullopt;
  if (src.display_labels_.has_value()) {
    out_display_labels = src.display_labels_->take(indices).label().reset();
  }

  return Slate(build_impl(std::move(out_payload),
                           std::move(out_mask),
                           std::move(out_display_labels),
                           src.display_name_,
                           src.label_kind_));
}

Slate Slate::filter(const pluss::Series& bool_mask) const {
  const auto indices = true_indices_from_mask(bool_mask, size());
  return take(indices);
}

Slate Slate::filter(const Slate& bool_mask) const {
  PLUSS_CHECK(bool_mask.kind() == SlateKind::Series,
              "Slate::filter: bool mask must be a Bool-typed Series slate");
  return filter(bool_mask.as_series());
}

Slate Slate::dropnull() const {
  if (!has_validity_mask()) {
    return *this;
  }
  const auto valid = validity_mask()->host<bool>();
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

Slate Slate::arrange(bool ascending, bool nulls_last) const {
  if (size() <= 1) {
    return *this;
  }

  if (kind() == SlateKind::Series && is_numeric_series_type(as_series())) {
    const auto values = as_series();

    if (!has_validity_mask()) {
      const auto order = values.argsort(/*dim=*/0, /*descending=*/!ascending, /*stable=*/true)
                             .host<std::int64_t>();
      return take(order);
    }

    const auto valid = validity_mask()->host<bool>();
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
                                         .host<std::int64_t>();
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

  if (kind() == SlateKind::Timeline) {
    const auto ticks = as_timeline().ticks();
    std::optional<std::vector<bool>> valid = std::nullopt;
    if (has_validity_mask()) {
      valid = validity_mask()->host<bool>();
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

  if (kind() == SlateKind::Note || (kind() == SlateKind::Series && !is_numeric_series_type(as_series()))) {
    std::vector<std::string> keys;
    keys.reserve(static_cast<std::size_t>(size()));
    if (kind() == SlateKind::Note) {
      const auto& note = as_note();
      for (std::int64_t i = 0; i < size(); ++i) {
        keys.emplace_back(note.at(i));
      }
    } else {
      for (std::int64_t i = 0; i < size(); ++i) {
        keys.emplace_back(slate_scalar_string_at(*this, i));
      }
    }

    std::optional<std::vector<bool>> valid = std::nullopt;
    if (has_validity_mask()) {
      valid = validity_mask()->host<bool>();
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

SlateSortBuilder Slate::sort(bool ascending, bool nulls_last) const {
  return SlateSortBuilder(*this, ascending, nulls_last);
}

SlateSortBuilder::SlateSortBuilder(Slate base, bool ascending, bool nulls_last)
    : base_(std::move(base)), ascending_(ascending), nulls_last_(nulls_last) {}

Slate SlateSortBuilder::asc() const {
  return base_.arrange(true, nulls_last_);
}

Slate SlateSortBuilder::desc() const {
  return base_.arrange(false, nulls_last_);
}

Slate SlateSortBuilder::done() const {
  return base_.arrange(ascending_, nulls_last_);
}

SlateSortBuilder::operator Slate() const {
  return done();
}

Slate Slate::ffill(std::int64_t limit) const {
  return forward_fill_slate(*this, limit);
}

Slate Slate::bfill(std::int64_t limit) const {
  return backward_fill_slate(*this, limit);
}

Slate Slate::interp(std::int64_t limit) const {
  PLUSS_CHECK(limit == -1 || limit >= 0, "Slate::interp: limit must be -1 or >= 0");
  PLUSS_CHECK(kind() == SlateKind::Series && is_numeric_series_type(as_series()),
              "Slate::interp is only valid for numeric Series payload");
  if (limit == 0 || size() == 0) {
    return *this;
  }
  auto valid = notnull().as_series().host<bool>();
  if (all_true(valid)) {
    return *this;
  }

  const auto source = as_series().to(pluss::codem::ScalarType::Float64);
  const auto platform = as_series().platform();
  const auto values = source.host<double>();
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
    return Slate(series);
  }
  auto mask = bool_series_from_bools(valid);
  if (mask.platform() != platform) {
    mask = mask.to(platform);
  }
  return Slate(series, mask);
}

Slate Slate::bucketize(pluss::msharp::RangeView::Double bins,
                         pluss::msharp::RangeView::Int labels,
                         bool right,
                         bool include_lowest) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::bucketize is only valid for Series payload");
  PLUSS_CHECK(is_numeric_series_type(as_series()),
              "Slate::bucketize is only valid for numeric Series payload");

  std::vector<double> edges(bins.begin(), bins.end());
  PLUSS_CHECK(edges.size() >= 2, "Slate::bucketize: bins must contain at least two edges");
  for (std::size_t i = 0; i < edges.size(); ++i) {
    PLUSS_CHECK(!std::isnan(edges[i]),
                "Slate::bucketize: bin edges must not be NaN");
    if (i > 0) {
      PLUSS_CHECK(edges[i] > edges[i - 1],
                  "Slate::bucketize: bin edges must be strictly increasing");
    }
  }

  const auto bucket_count = static_cast<std::int64_t>(edges.size() - 1);
  std::vector<std::int64_t> label_values;
  if (labels.size() == 0) {
    label_values.resize(static_cast<std::size_t>(bucket_count));
    std::iota(label_values.begin(), label_values.end(), static_cast<std::int64_t>(0));
  } else {
    PLUSS_CHECK(static_cast<std::int64_t>(labels.size()) == bucket_count,
                "Slate::bucketize: labels size must equal bins.size() - 1");
    label_values.assign(labels.begin(), labels.end());
  }

  auto x = as_series().to(pluss::codem::ScalarType::Float64);
  const auto platform = x.platform();
  auto out = pluss::Series::full({x.numel()},
                                 static_cast<double>(label_values.front()),
                                 pluss::codem::ScalarType::Int64).to(platform);
  auto assigned = pluss::Series::zeros({x.numel()}, pluss::codem::ScalarType::Bool).to(platform);

  pluss::Series base_valid = pluss::series::undefined();
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

    pluss::Series lower = pluss::series::undefined();
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
    return Slate(out);
  }
  return Slate(out, assigned);
}

Slate Slate::distinct(bool dropnull) const {
  std::vector<std::int64_t> keep_indices;
  keep_indices.reserve(static_cast<std::size_t>(size()));

  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(size()));
  bool null_emitted = false;

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->host<bool>();
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

    const auto key = slate_scalar_string_at(*this, i);
    if (seen.insert(key).second) {
      keep_indices.push_back(i);
    }
  }

  return take(keep_indices);
}

Slate Slate::freq() const {
  return freq(SlateFreqOptions{});
}

Slate Slate::freq(SlateFreqOptions options) const {
  PLUSS_CHECK(options.top >= 0, "Slate::freq: top must be >= 0");

  struct FreqRow {
    std::string key;
    std::int64_t count{0};
  };

  std::vector<FreqRow> rows;
  rows.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, size() / 8)));
  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(rows.capacity());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->host<bool>();
  }

  for (std::int64_t i = 0; i < size(); ++i) {
    const bool is_valid = !valid.has_value() || (*valid)[static_cast<std::size_t>(i)];
    if (!is_valid && options.dropnull) {
      continue;
    }
    const std::string key = is_valid ? slate_scalar_string_at(*this, i) : "NULL";
    const auto [it, inserted] = index_by_key.emplace(key, rows.size());
    if (inserted) {
      rows.push_back(FreqRow{key, 1});
    } else {
      rows[it->second].count += 1;
    }
  }

  if (options.sort) {
    std::stable_sort(rows.begin(), rows.end(), [&options](const FreqRow& lhs, const FreqRow& rhs) {
      if (lhs.count == rhs.count) {
        return false;
      }
      return options.ascending ? (lhs.count < rhs.count) : (lhs.count > rhs.count);
    });
  }
  if (options.top > 0 && static_cast<std::size_t>(options.top) < rows.size()) {
    rows.resize(static_cast<std::size_t>(options.top));
  }

  std::vector<std::string> labels;
  labels.reserve(rows.size());
  std::vector<std::int64_t> counts;
  counts.reserve(rows.size());
  for (const auto& row : rows) {
    labels.push_back(row.key);
    counts.push_back(row.count);
  }

  auto label_slate = Slate(make_utf8_note(labels));
  auto count_series = pluss::Series::empty({static_cast<std::int64_t>(counts.size())},
                                           pluss::codem::ScalarType::Int64);
  if (!counts.empty()) {
    std::memcpy(count_series.raw_data(), counts.data(), counts.size() * sizeof(std::int64_t));
  }
  auto count_slate = Slate(count_series).label().set(label_slate);
  if (options.kind == SlateFreqKind::Prop) {
    if (count_slate.size() == 0) {
      return Slate(pluss::Series::empty({0}, pluss::codem::ScalarType::Float64))
          .label().set(label_slate)
          .name("prop");
    }
    const auto total =
        static_cast<double>(count_slate.sum().slate().as_series().item<std::int64_t>());
    return count_slate.div(total).label().set(label_slate).name("prop");
  }
  return count_slate.name("count");
}

Slate detail::freq_count_slate(const Slate& slate,
                                 std::int64_t top_n,
                                 bool dropnull,
                                 std::string_view display_name) {
  auto out = slate.freq(SlateFreqOptions{.dropnull = dropnull, .top = top_n});
  if (!display_name.empty()) {
    out = out.name(std::string(display_name));
  }
  return out;
}

SlateValue Slate::ndistinct(bool dropnull) const {
  auto value = pluss::Series::empty({1}, pluss::codem::ScalarType::Int64);
  reinterpret_cast<std::int64_t*>(value.raw_data())[0] = distinct(dropnull).size();
  return SlateValue(Slate(value));
}

Slate Slate::duplicated(bool keep_first, bool keep_last) const {
  PLUSS_CHECK(!(keep_first && keep_last),
              "Slate::duplicated: keep_first and keep_last cannot both be true");
  std::vector<bool> duplicated_mask(static_cast<std::size_t>(size()), false);
  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->host<bool>();
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_key;
  rows_by_key.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t row = 0; row < size(); ++row) {
    const bool observed = !valid.has_value() || (*valid)[static_cast<std::size_t>(row)];
    const auto key = observed ? slate_scalar_string_at(*this, row) : std::string{"NULL"};
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
  return Slate(bool_series_from_bools(duplicated_mask));
}

Slate Slate::repeated() const {
  std::vector<bool> repeated_mask(static_cast<std::size_t>(size()), false);
  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->host<bool>();
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_key;
  rows_by_key.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t row = 0; row < size(); ++row) {
    const bool observed = !valid.has_value() || (*valid)[static_cast<std::size_t>(row)];
    const auto key = observed ? slate_scalar_string_at(*this, row) : std::string{"NULL"};
    rows_by_key[key].push_back(row);
  }

  for (const auto& [_, rows] : rows_by_key) {
    if (rows.size() <= 1) {
      continue;
    }
    for (const auto row : rows) {
      repeated_mask[static_cast<std::size_t>(row)] = true;
    }
  }
  return Slate(bool_series_from_bools(repeated_mask));
}

SlateView Slate::describe(bool render) const {
  const auto name = named() ? this->name() : std::string("value");

  if (kind() == SlateKind::Series && is_numeric_series_type(as_series())) {
    auto desc_frame = DataFrame({{name, *this}}).describe(false);
    auto out = desc_frame.features(name);
    return SlateView(std::move(out), render, "describe");
  }

  if (kind() == SlateKind::Note) {
    const auto non_null = count(true).as_series().item<std::int64_t>();
    const auto distinct_count = distinct(/*dropnull=*/true).count(true).as_series().item<std::int64_t>();
    auto counts = freq();

    std::string top_value = "NULL";
    std::int64_t top_freq = 0;
    if (counts.size() > 0) {
      top_value = slate_scalar_string_at(counts.label().values(), 0);
      top_freq = counts.as_series().host<std::int64_t>().front();
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

    auto out = Slate(payload)
                   .label().set(Slate(make_utf8_note({"count", "unique", "top", "freq"})))
                   .name(name);
    return SlateView(std::move(out), render, "describe");
  }

  PLUSS_FAIL("Slate::describe supports numeric Series and Note payloads");
}

SlateValue Slate::sum() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::sum is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Slate::sum on an empty (all-null) selection");
  return SlateValue(Slate(s.sum().reshape({1})));
}

SlateValue Slate::prod() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::prod is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(notnull().as_series());
  }
  PLUSS_CHECK(s.numel() > 0, "Slate::prod on an empty (all-null) selection");
  return SlateValue(Slate(s.prod().reshape({1})));
}

SlateValue Slate::count(bool count_true) const {
  if (kind() == SlateKind::Series) {
    auto s = as_series();
    if (has_validity_mask()) {
      s = s.masked_select(notnull().as_series());
    }
    return SlateValue(Slate(s.count(count_true).reshape({1})));
  }

  PLUSS_CHECK(count_true, "Slate::count(false) is only valid for Bool Series payload");
  std::int64_t total = size();
  if (has_validity_mask()) {
    const auto valid = notnull().as_series().host<bool>();
    total = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  }
  auto out = pluss::Series::empty({1}, pluss::codem::ScalarType::Int64);
  reinterpret_cast<std::int64_t*>(out.raw_data())[0] = total;
  return SlateValue(Slate(out));
}

SlateValue Slate::any(bool skipnull) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::any is only valid for Series payload");
  auto s = as_series();
  PLUSS_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool,
              "Slate::any is only valid for Bool Series payload");

  if (has_validity_mask()) {
    PLUSS_CHECK(skipnull,
                "Slate::any(skipnull=false) is not supported for nullable Bool Series payload");
    const auto valid = notnull().as_series();
    s = s.logical_and(valid);
  }

  const auto true_count = s.count(true).item<std::int64_t>();
  return SlateValue(Slate(bool_series_from_bools({true_count > 0})));
}

SlateValue Slate::all(bool skipnull) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::all is only valid for Series payload");
  auto s = as_series();
  PLUSS_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool,
              "Slate::all is only valid for Bool Series payload");

  if (has_validity_mask()) {
    PLUSS_CHECK(skipnull,
                "Slate::all(skipnull=false) is not supported for nullable Bool Series payload");
    const auto valid = notnull().as_series();
    s = s.logical_or(valid.logical_not());
  }

  const auto true_count = s.count(true).item<std::int64_t>();
  return SlateValue(Slate(bool_series_from_bools({true_count == s.numel()})));
}

SlateValue Slate::mean() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::mean is only valid for Series payload");
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
  PLUSS_CHECK(s.numel() > 0, "Slate::mean on an empty (all-null) selection");
  return SlateValue(Slate(s.mean().reshape({1})));
}

SlateValue Slate::median() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::median is only valid for Series payload");
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
  PLUSS_CHECK(s.numel() > 0, "Slate::median on an empty (all-null) selection");
  return SlateValue(Slate(s.median().reshape({1})));
}

SlateValue Slate::min() const {
  auto observed = has_validity_mask() ? dropnull() : *this;
  PLUSS_CHECK(observed.size() > 0, "Slate::min on an empty (all-null) selection");
  switch (observed.kind()) {
    case SlateKind::Series:
      return SlateValue(Slate(observed.as_series().min().reshape({1})));
    case SlateKind::Note: {
      const auto sorted = observed.arrange(true, true);
      return SlateValue(Slate(make_utf8_note({std::string(sorted.as_note().at(0))})));
    }
    case SlateKind::Timeline: {
      const auto sorted = observed.arrange(true, true);
      const auto ticks = sorted.as_timeline().ticks();
      return SlateValue(Slate(rebuild_timeline_like(sorted.as_timeline(), {ticks.front()})));
    }
    case SlateKind::Duration: {
      const auto sorted = observed.arrange(true, true);
      return SlateValue(Slate(pluss::duration::nanos(
          std::vector<std::int64_t>{sorted.as_duration().host().front()})));
    }
  }
  PLUSS_FAIL("Slate::min: unsupported payload kind");
}

SlateValue Slate::max() const {
  auto observed = has_validity_mask() ? dropnull() : *this;
  PLUSS_CHECK(observed.size() > 0, "Slate::max on an empty (all-null) selection");
  switch (observed.kind()) {
    case SlateKind::Series:
      return SlateValue(Slate(observed.as_series().max().reshape({1})));
    case SlateKind::Note: {
      const auto sorted = observed.arrange(false, true);
      return SlateValue(Slate(make_utf8_note({std::string(sorted.as_note().at(0))})));
    }
    case SlateKind::Timeline: {
      const auto sorted = observed.arrange(false, true);
      const auto ticks = sorted.as_timeline().ticks();
      return SlateValue(Slate(rebuild_timeline_like(sorted.as_timeline(), {ticks.front()})));
    }
    case SlateKind::Duration: {
      const auto sorted = observed.arrange(false, true);
      return SlateValue(Slate(pluss::duration::nanos(
          std::vector<std::int64_t>{sorted.as_duration().host().front()})));
    }
  }
  PLUSS_FAIL("Slate::max: unsupported payload kind");
}

SlateValue Slate::csmin() const {
  PLUSS_CHECK(size() > 0, "Slate::csmin on an empty Slate");
  std::int64_t best = -1;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    if (best < 0 || compare_rows(row, best, true) < 0) {
      best = row;
    }
  }
  PLUSS_CHECK(best >= 0, "Slate::csmin on an all-null Slate");
  const std::array<std::int64_t, 1> index{best};
  return SlateValue(caselbl().take(std::span<const std::int64_t>(index.data(), index.size())));
}

SlateValue Slate::csmax() const {
  PLUSS_CHECK(size() > 0, "Slate::csmax on an empty Slate");
  std::int64_t best = -1;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    if (best < 0 || compare_rows(row, best, true) > 0) {
      best = row;
    }
  }
  PLUSS_CHECK(best >= 0, "Slate::csmax on an all-null Slate");
  const std::array<std::int64_t, 1> index{best};
  return SlateValue(caselbl().take(std::span<const std::int64_t>(index.data(), index.size())));
}

SlateValue Slate::var(bool unbiased) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::var is only valid for Series payload");
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
  PLUSS_CHECK(s.numel() > 0, "Slate::var on an empty (all-null) selection");
  return SlateValue(Slate(s.var(unbiased).reshape({1})));
}

SlateValue Slate::std(bool unbiased) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::std is only valid for Series payload");
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
  PLUSS_CHECK(s.numel() > 0, "Slate::std on an empty (all-null) selection");
  return SlateValue(Slate(s.std(unbiased).reshape({1})));
}

double Slate::corr(const Slate& other, SlateCorrMethod method) const {
  PLUSS_CHECK(kind() == SlateKind::Series && other.kind() == SlateKind::Series,
              "Slate::corr: both operands must be Series");
  PLUSS_CHECK(is_numeric_series_type(as_series()) && is_numeric_series_type(other.as_series()),
              "Slate::corr: both operands must be numeric Series");
  const auto aligned = align_slate_pair_for_cases_(*this, other, "Slate::corr");
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
    case SlateCorrMethod::Pearson: {
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
    case SlateCorrMethod::Spearman: {
      const auto x = lhs_valid.host<double>();
      const auto y = rhs_valid.host<double>();
      const auto rx = rank_average_ties_(x);
      const auto ry = rank_average_ties_(y);
      return pearson_corr_from_vectors_(rx, ry);
    }
    case SlateCorrMethod::KendallTau: {
      const auto x = lhs_valid.host<double>();
      const auto y = rhs_valid.host<double>();
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

Slate Slate::lower() const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::lower: expected Note payload");
  return Slate(as_note().lower(), validity_mask());
}

Slate Slate::strip() const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::strip is only valid for Note payload");
  return Slate(as_note().strip(), validity_mask());
}

Slate Slate::strip(std::string_view chars) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::strip(chars) is only valid for Note payload");
  return Slate(as_note().strip(chars), validity_mask());
}

Slate Slate::lstrip() const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::lstrip is only valid for Note payload");
  return Slate(as_note().lstrip(), validity_mask());
}

Slate Slate::lstrip(std::string_view chars) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::lstrip(chars) is only valid for Note payload");
  return Slate(as_note().lstrip(chars), validity_mask());
}

Slate Slate::rstrip() const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::rstrip is only valid for Note payload");
  return Slate(as_note().rstrip(), validity_mask());
}

Slate Slate::rstrip(std::string_view chars) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::rstrip(chars) is only valid for Note payload");
  return Slate(as_note().rstrip(chars), validity_mask());
}

namespace {

std::optional<pluss::Series> nullify_matches_mask(const Slate& t, const pluss::Series& matches, bool& changed) {
  auto valid = detail::has_validity_mask(t) ? detail::validity_mask(t)->host<bool>()
                                           : std::vector<bool>(static_cast<std::size_t>(t.size()), true);
  const auto hit = matches.host<bool>();

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

Slate Slate::replace(std::string_view from, std::string_view to) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::replace: expected Note payload");
  return Slate(as_note().replace(from, to), validity_mask());
}

Slate Slate::replace(double from, double to) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::replace(double, double) is only valid for Series payload");
  auto mask = compare(from, SlateBinaryCompareOp::Eq).as_series();
  return Slate(as_series().where(mask, to), validity_mask());
}

Slate Slate::replace(double from, pluss::null_t) const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::replace(double, null) is only valid for Series payload");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, compare(from, SlateBinaryCompareOp::Eq).as_series(), changed);
  if (!changed) {
    return *this;
  }
  return Slate(as_series(), new_mask);
}

Slate Slate::replace(pluss::null_t, double to) const {
  return fillnull(to);
}

Slate Slate::replace(const pluss::Series& from, const pluss::Series& to) const {
  PLUSS_CHECK(from.defined(), "Slate::replace(Series, Series): from Series is undefined");
  PLUSS_CHECK(to.defined(), "Slate::replace(Series, Series): to Series is undefined");
  PLUSS_CHECK(from.numel() == 1, "Slate::replace(Series, Series): from Series must be scalar");
  PLUSS_CHECK(to.numel() == 1, "Slate::replace(Series, Series): to Series must be scalar");
  double lhs = 0.0;
  double rhs = 0.0;
  PLUSS_CHECK(extract_numeric_value(from, 0, lhs),
              "Slate::replace(Series, Series): from Series must be numeric or bool");
  PLUSS_CHECK(extract_numeric_value(to, 0, rhs),
              "Slate::replace(Series, Series): to Series must be numeric or bool");
  return replace(lhs, rhs);
}

Slate Slate::replace(const pluss::Series& from, pluss::null_t to) const {
  PLUSS_CHECK(from.defined(), "Slate::replace(Series, null): from Series is undefined");
  PLUSS_CHECK(from.numel() == 1, "Slate::replace(Series, null): from Series must be scalar");
  double lhs = 0.0;
  PLUSS_CHECK(extract_numeric_value(from, 0, lhs),
              "Slate::replace(Series, null): from Series must be numeric or bool");
  return replace(lhs, to);
}

Slate Slate::replace(pluss::null_t from, const pluss::Series& to) const {
  PLUSS_CHECK(to.defined(), "Slate::replace(null, Series): to Series is undefined");
  PLUSS_CHECK(to.numel() == 1, "Slate::replace(null, Series): to Series must be scalar");
  double rhs = 0.0;
  PLUSS_CHECK(extract_numeric_value(to, 0, rhs),
              "Slate::replace(null, Series): to Series must be numeric or bool");
  return replace(from, rhs);
}

Slate Slate::replace(const pluss::Note& from, const pluss::Note& to) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::replace(Note, Note) is only valid for Note payload");
  PLUSS_CHECK(from.defined(), "Slate::replace(Note, Note): from Note is undefined");
  PLUSS_CHECK(to.defined(), "Slate::replace(Note, Note): to Note is undefined");
  PLUSS_CHECK(from.numel() == 1, "Slate::replace(Note, Note): from Note must be scalar");
  PLUSS_CHECK(to.numel() == 1, "Slate::replace(Note, Note): to Note must be scalar");

  const auto from_text = std::string(from.at(0));
  const auto to_text = std::string(to.at(0));
  const auto valid = has_validity_mask() ? validity_mask()->host<bool>()
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
  return Slate(rebuild_note_like(as_note(), values), validity_mask());
}

Slate Slate::replace(const pluss::Note& from, pluss::null_t to) const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate::replace(Note, null) is only valid for Note payload");
  PLUSS_CHECK(from.defined(), "Slate::replace(Note, null): from Note is undefined");
  PLUSS_CHECK(from.numel() == 1, "Slate::replace(Note, null): from Note must be scalar");

  const auto from_text = std::string(from.at(0));
  auto valid = has_validity_mask() ? validity_mask()->host<bool>()
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
  return Slate(as_note(), optional_mask_from_valid(valid));
}

Slate Slate::replace(pluss::null_t, const pluss::Note& to) const {
  PLUSS_CHECK(to.defined(), "Slate::replace(null, Note): to Note is undefined");
  PLUSS_CHECK(to.numel() == 1, "Slate::replace(null, Note): to Note must be scalar");
  return fillnull(to.at(0));
}

Slate Slate::replace(const pluss::Timeline& from, const pluss::Timeline& to) const {
  PLUSS_CHECK(kind() == SlateKind::Timeline,
              "Slate::replace(Timeline, Timeline) is only valid for Timeline payload");
  PLUSS_CHECK(from.defined(), "Slate::replace(Timeline, Timeline): from Timeline is undefined");
  PLUSS_CHECK(to.defined(), "Slate::replace(Timeline, Timeline): to Timeline is undefined");
  PLUSS_CHECK(from.numel() == 1, "Slate::replace(Timeline, Timeline): from Timeline must be scalar");
  PLUSS_CHECK(to.numel() == 1, "Slate::replace(Timeline, Timeline): to Timeline must be scalar");

  const auto keep = compare(from, SlateBinaryCompareOp::Eq).as_series().logical_not();
  return Slate(as_timeline().where(keep, to), validity_mask());
}

Slate Slate::replace(const pluss::Timeline& from, pluss::null_t to) const {
  PLUSS_CHECK(kind() == SlateKind::Timeline,
              "Slate::replace(Timeline, null) is only valid for Timeline payload");
  PLUSS_CHECK(from.defined(), "Slate::replace(Timeline, null): from Timeline is undefined");
  PLUSS_CHECK(from.numel() == 1, "Slate::replace(Timeline, null): from Timeline must be scalar");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(*this, compare(from, SlateBinaryCompareOp::Eq).as_series(), changed);
  if (!changed) {
    return *this;
  }
  return Slate(as_timeline(), new_mask);
}

Slate Slate::replace(pluss::null_t, const pluss::Timeline& to) const {
  return fillnull(to);
}

Slate Slate::replace(
    const pluss::Duration& from,
    const pluss::Duration& to) const {
  PLUSS_CHECK(kind() == SlateKind::Duration,
              "Slate::replace(Duration, Duration) is only valid for Duration payload");
  PLUSS_CHECK(from.defined() && from.numel() == 1,
              "Slate::replace(Duration, Duration): from must contain exactly one value");
  PLUSS_CHECK(to.defined() && to.numel() == 1,
              "Slate::replace(Duration, Duration): to must contain exactly one value");
  const auto from_scalar = pluss::duration::nanos(from.host().front());
  const auto to_scalar = pluss::duration::nanos(to.host().front());
  return Slate(
      as_duration().where(as_duration().ne(from_scalar), to_scalar),
      validity_mask());
}

Slate Slate::replace(const pluss::Duration& from, pluss::null_t) const {
  PLUSS_CHECK(kind() == SlateKind::Duration,
              "Slate::replace(Duration, null) is only valid for Duration payload");
  PLUSS_CHECK(from.defined() && from.numel() == 1,
              "Slate::replace(Duration, null): from must contain exactly one value");
  bool changed = false;
  const auto new_mask = nullify_matches_mask(
      *this, compare(from, SlateBinaryCompareOp::Eq).as_series(), changed);
  return changed ? Slate(as_duration(), new_mask) : *this;
}

Slate Slate::replace(pluss::null_t, const pluss::Duration& to) const {
  return fillnull(to);
}

Slate Slate::has(double value) const {
  return eq(value);
}

Slate Slate::has(std::string_view value) const {
  return eq(value);
}

Slate Slate::has(const Slate& values) const {
  PLUSS_CHECK(values.defined(), "Slate::has: values Slate is undefined");
  auto out = Slate(pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool));
  if (detail::has_display_labels(*this)) {
    out = detail::with_display_labels(out, detail::display_labels(*this));
  }
  // The numeric fast path routes through has(double) -> eq(double), which only
  // accepts a Series-backed receiver. Take it only when *this* is numeric too;
  // otherwise fall back to string-form comparison (matching how freq()/
  // distinct()/duplicated() group across payload kinds) so that, e.g.,
  // note_slate.has(numeric_slate) yields all-false instead of throwing.
  const bool self_numeric = kind() == SlateKind::Series && is_numeric_series_type(as_series());
  for (std::int64_t i = 0; i < values.size(); ++i) {
    if (!values.is_valid(i)) {
      continue;
    }
    if (self_numeric && values.kind() == SlateKind::Series &&
        is_numeric_series_type(values.as_series())) {
      double scalar = 0.0;
      PLUSS_CHECK(extract_numeric_value(values.as_series(), i, scalar),
                  "Slate::has: failed to extract numeric membership value");
      out = out.lor(has(scalar));
    } else {
      out = out.lor(has(slate_scalar_string_at(values, i)));
    }
  }
  return out;
}

Slate Slate::year() const {
  if (kind() == SlateKind::Timeline) {
    return Slate(as_timeline().year(), validity_mask());
  }
  PLUSS_FAIL("Slate::year is only valid for Timeline payload");
}

Slate Slate::month() const {
  if (kind() == SlateKind::Timeline) {
    return Slate(as_timeline().month(), validity_mask());
  }
  PLUSS_FAIL("Slate::month is only valid for Timeline payload");
}

Slate Slate::day() const {
  if (kind() == SlateKind::Timeline) {
    return Slate(as_timeline().day(), validity_mask());
  }
  PLUSS_FAIL("Slate::day is only valid for Timeline payload");
}

Slate Slate::hour() const {
  if (kind() == SlateKind::Timeline) {
    return Slate(as_timeline().hour(), validity_mask());
  }
  PLUSS_FAIL("Slate::hour is only valid for Timeline payload");
}

Slate Slate::math_(double other, SlateBinaryMathOp op, bool reverse) const {
  if (kind() != SlateKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  return carry_case_labels_for_alignment_(
      Slate(apply_math_series_scalar(as_series(), other, op, reverse), validity_mask()),
      *this);
}

Slate Slate::math_(const Slate& other, SlateBinaryMathOp op) const {
  if (kind() != SlateKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  PLUSS_CHECK(other.kind() == SlateKind::Series && is_numeric_series_type(other.as_series()),
              "Slate::math_: rhs must be numeric Series");
  return aligned_numeric_binary_(*this, other, op, "Slate::math_");
}

Slate Slate::compare(double other, SlateBinaryCompareOp op) const {
  if (kind() == SlateKind::Series) {
    return carry_case_labels_for_alignment_(
        Slate(apply_compare_series_scalar(as_series(), other, op), validity_mask()),
        *this);
  }

  auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->host<bool>();
  }

  for (std::int64_t i = 0; i < size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      p[i] = false;
      continue;
    }
    const auto text = slate_scalar_string_at(*this, i);
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
  return carry_case_labels_for_alignment_(Slate(out, validity_mask()), *this);
}

Slate Slate::compare(std::string_view other, SlateBinaryCompareOp op) const {
  if (kind() == SlateKind::Note) {
    pluss::Note raw = pluss::Note::undefined();
    switch (op) {
      case SlateBinaryCompareOp::Eq:
        raw = (as_note() == other);
        break;
      case SlateBinaryCompareOp::Ne:
        raw = (as_note() != other);
        break;
      case SlateBinaryCompareOp::Lt:
        raw = (as_note() < other);
        break;
      case SlateBinaryCompareOp::Le:
        raw = (as_note() <= other);
        break;
      case SlateBinaryCompareOp::Gt:
        raw = (as_note() > other);
        break;
      case SlateBinaryCompareOp::Ge:
        raw = (as_note() >= other);
        break;
    }
    return carry_case_labels_for_alignment_(Slate(bool_series_from_u8(raw), validity_mask()), *this);
  }

  if (kind() == SlateKind::Timeline) {
    try {
      return compare(pluss::make_timeline(other), op);
    } catch (const std::exception& e) {
      PLUSS_FAIL("Slate::compare(string): temporal rhs literal '" + std::string(other)
                 + "' could not be parsed as Timeline: " + e.what());
    }
  }

  auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->host<bool>();
  }

  const std::string rhs_text(other);
  for (std::int64_t i = 0; i < size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      p[i] = false;
      continue;
    }
    const auto lhs_text = slate_scalar_string_at(*this, i);
    switch (op) {
      case SlateBinaryCompareOp::Eq:
        p[i] = lhs_text == rhs_text;
        break;
      case SlateBinaryCompareOp::Ne:
        p[i] = lhs_text != rhs_text;
        break;
      case SlateBinaryCompareOp::Lt:
        p[i] = lhs_text < rhs_text;
        break;
      case SlateBinaryCompareOp::Le:
        p[i] = lhs_text <= rhs_text;
        break;
      case SlateBinaryCompareOp::Gt:
        p[i] = lhs_text > rhs_text;
        break;
      case SlateBinaryCompareOp::Ge:
        p[i] = lhs_text >= rhs_text;
        break;
    }
  }
  return carry_case_labels_for_alignment_(Slate(out, validity_mask()), *this);
}

Slate Slate::compare(const pluss::Series& other, SlateBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Slate::compare(Series): rhs Series is undefined");
  if (other.dim() == 0) {
    double rhs_value = 0.0;
    PLUSS_CHECK(extract_numeric_value(other, 0, rhs_value),
                "Slate::compare(Series): scalar rhs Series must be numeric or bool");
    return compare(rhs_value, op);
  }
  PLUSS_CHECK(other.dim() == 1, "Slate::compare(Series): rhs Series must be scalar or 1D");
  if (other.numel() != size()) {
    throw ::pluss::Error("Slate::compare(Series): positional comparison requires identical lengths");
  }
  return compare(Slate(other), op);
}

Slate Slate::compare(const pluss::Note& other, SlateBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Slate::compare(Note): rhs Note is undefined");
  if (other.dim() == 0) {
    return compare(other.at(0), op);
  }
  PLUSS_CHECK(other.dim() == 1, "Slate::compare(Note): rhs Note must be scalar or 1D");
  if (other.numel() != size()) {
    throw ::pluss::Error("Slate::compare(Note): positional comparison requires identical lengths");
  }
  return compare(Slate(other), op);
}

Slate Slate::compare(const pluss::Timeline& other, SlateBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Slate::compare(Timeline): rhs Timeline is undefined");
  if (other.dim() == 0 && kind() == SlateKind::Timeline) {
    return carry_case_labels_for_alignment_(
        Slate(apply_compare_timeline_timeline(as_timeline(), other, op), validity_mask()),
        *this);
  }
  PLUSS_CHECK(other.dim() == 1, "Slate::compare(Timeline): rhs Timeline must be scalar or 1D");
  if (other.numel() != size()) {
    throw ::pluss::Error("Slate::compare(Timeline): positional comparison requires identical lengths");
  }
  return compare(Slate(other), op);
}

Slate Slate::compare(const pluss::Duration& other, SlateBinaryCompareOp op) const {
  PLUSS_CHECK(other.defined(), "Slate::compare(Duration): rhs Duration is undefined");
  PLUSS_CHECK(kind() == SlateKind::Duration,
              "Slate::compare(Duration): lhs must be Duration-backed");

  if (other.dim() == 0 || other.numel() == 1) {
    const auto scalar = other.host().front();
    const auto expanded = pluss::duration::nanos(
        std::vector<std::int64_t>(static_cast<std::size_t>(size()), scalar));
    return compare(Slate(expanded), op);
  }
  PLUSS_CHECK(other.dim() == 1,
              "Slate::compare(Duration): rhs Duration must be scalar or 1D");
  PLUSS_CHECK(other.numel() == size(),
              "Slate::compare(Duration): positional comparison requires identical lengths");
  return compare(Slate(other), op);
}

Slate Slate::compare(const Slate& other, SlateBinaryCompareOp op) const {
  if (other.size() != size()) {
    throw ::pluss::Error("Slate::compare: positional alignment requires identical lengths");
  }
  return aligned_compare_(*this, other, op, "Slate::compare");
}

Slate Slate::between(double a, double b, bool inclusive) const {
  return between(a, b, inclusive, inclusive);
}

Slate Slate::between(double a, double b, bool include_left, bool include_right) const {
  PLUSS_CHECK(kind() == SlateKind::Series,
              "Slate::between(double,double,...): requires Series payload; use temporal overloads for date/time slates");
  PLUSS_CHECK(is_numeric_series_type(as_series()),
              "Slate::between(double,double,...): Series payload must be numeric");

  auto lower = a;
  auto upper = b;
  if (lower > upper) {
    std::swap(lower, upper);
    std::swap(include_left, include_right);
  }

  const auto left = include_left ? compare(lower, SlateBinaryCompareOp::Ge)
                                 : compare(lower, SlateBinaryCompareOp::Gt);
  const auto right = include_right ? compare(upper, SlateBinaryCompareOp::Le)
                                   : compare(upper, SlateBinaryCompareOp::Lt);
  return (left & right);
}

Slate Slate::between(std::string_view lo, std::string_view hi, bool inclusive) const {
  return between(lo, hi, inclusive, inclusive);
}

Slate Slate::between(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const {
  PLUSS_CHECK(kind() == SlateKind::Timeline,
              "Slate::between(string,string,...): requires Timeline payload");

  auto lower = pluss::make_timeline(lo);
  auto upper = pluss::make_timeline(hi);
  if ((lower > upper).item<bool>()) {
    std::swap(lower, upper);
    std::swap(include_left, include_right);
  }

  const auto axis = as_timeline();
  const auto left = include_left ? axis.ge(lower) : axis.gt(lower);
  const auto right = include_right ? axis.le(upper) : axis.lt(upper);
  return Slate(left.logical_and(right), validity_mask());
}

Slate Slate::between(const Slate& lo, const Slate& hi, bool inclusive) const {
  return between(lo, hi, inclusive, inclusive);
}

Slate Slate::between(const Slate& lo, const Slate& hi, bool include_left, bool include_right) const {
  PLUSS_CHECK(lo.defined() && hi.defined(), "Slate::between(Slate,Slate,...): lower/upper bounds must be defined");
  const auto left = include_left ? (*this >= lo) : (*this > lo);
  const auto right = include_right ? (*this <= hi) : (*this < hi);
  return (left & right);
}

int Slate::compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last) const {
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
    case SlateKind::Series: {
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
      return cmp_string(slate_scalar_string_at(*this, l), slate_scalar_string_at(*this, r));
    }
    case SlateKind::Note:
      return cmp_string(std::string(as_note().at(l)), std::string(as_note().at(r)));
    case SlateKind::Timeline: {
      const auto ticks = as_timeline().ticks();
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
    case SlateKind::Duration: {
      const auto nanos = as_duration().host();
      const auto lv_nanos = nanos[static_cast<std::size_t>(l)];
      const auto rv_nanos = nanos[static_cast<std::size_t>(r)];
      if (lv_nanos < rv_nanos) {
        return -1;
      }
      if (lv_nanos > rv_nanos) {
        return 1;
      }
      return 0;
    }
  }
  PLUSS_FAIL("Slate::compare_rows: unsupported payload kind");
}

Slate Slate::window(std::int64_t window_size, SlateWindowOp op, std::int64_t min_periods) const {
  PLUSS_CHECK(window_size > 0, "Slate::window: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "Slate::window: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window_size, "Slate::window: min_periods cannot exceed window");
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate::window: only Series payload supports rolling ops");
  PLUSS_CHECK(is_numeric_series_type(as_series()), "Slate::window: Series payload must be numeric");

  const auto n = size();
  auto out = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  auto* out_data = reinterpret_cast<double*>(out.raw_data());
  std::vector<bool> out_valid(static_cast<std::size_t>(n), false);

  std::vector<bool> in_valid(static_cast<std::size_t>(n), true);
  if (has_validity_mask()) {
    in_valid = validity_mask()->host<bool>();
  }

  std::vector<double> values(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    if (!in_valid[static_cast<std::size_t>(i)]) {
      continue;
    }
    double v = 0.0;
    const bool ok = extract_numeric_value(as_series(), i, v);
    PLUSS_CHECK(ok, "Slate::window: failed numeric extraction");
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

  if (op == SlateWindowOp::RollingSum || op == SlateWindowOp::RollingMean) {
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
      out_data[i] = (op == SlateWindowOp::RollingMean)
                        ? (sum / static_cast<double>(cnt))
                        : sum;
      out_valid[static_cast<std::size_t>(i)] = true;
    }
    return Slate(out, optional_mask_from_valid(out_valid));
  }

  const bool is_min = (op == SlateWindowOp::RollingMin);
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

  return Slate(out, optional_mask_from_valid(out_valid));
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

bool cast_parse_ndigits_(std::string_view text,
                         std::size_t start,
                         std::size_t count,
                         int* out) {
  PLUSS_CHECK(out != nullptr, "cast_parse_ndigits_: output pointer is null");
  if (start + count > text.size()) return false;
  int value = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const char ch = text[start + i];
    if (ch < '0' || ch > '9') return false;
    value = value * 10 + static_cast<int>(ch - '0');
  }
  *out = value;
  return true;
}

std::optional<std::int64_t> cast_timeline_tick_from_fixed_pattern_(
    std::string_view text,
    std::string_view pattern) {
  auto build = [](int y, int mo, int d, int hh, int mm, int ss) -> std::optional<std::int64_t> {
    const auto ymd = std::chrono::year(y) / std::chrono::month(static_cast<unsigned>(mo)) /
        std::chrono::day(static_cast<unsigned>(d));
    if (!ymd.ok()) return std::nullopt;
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 60) return std::nullopt;
    const auto seconds =
        cast_days_from_civil_(y, static_cast<unsigned>(mo), static_cast<unsigned>(d)) * 86'400LL +
        static_cast<std::int64_t>(hh) * 3'600LL +
        static_cast<std::int64_t>(mm) * 60LL +
        static_cast<std::int64_t>(ss);
    if (seconds < std::numeric_limits<std::int64_t>::min() / 1'000'000'000LL ||
        seconds > std::numeric_limits<std::int64_t>::max() / 1'000'000'000LL) {
      return std::nullopt;
    }
    return seconds * 1'000'000'000LL;
  };

  int y = 0;
  int mo = 0;
  int d = 0;
  int hh = 0;
  int mm = 0;
  int ss = 0;

  if (pattern == "%Y-%m-%d") {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return std::nullopt;
    if (!cast_parse_ndigits_(text, 0, 4, &y) ||
        !cast_parse_ndigits_(text, 5, 2, &mo) ||
        !cast_parse_ndigits_(text, 8, 2, &d)) {
      return std::nullopt;
    }
    return build(y, mo, d, 0, 0, 0);
  }

  if (pattern == "%Y/%m/%d") {
    if (text.size() != 10 || text[4] != '/' || text[7] != '/') return std::nullopt;
    if (!cast_parse_ndigits_(text, 0, 4, &y) ||
        !cast_parse_ndigits_(text, 5, 2, &mo) ||
        !cast_parse_ndigits_(text, 8, 2, &d)) {
      return std::nullopt;
    }
    return build(y, mo, d, 0, 0, 0);
  }

  if (pattern == "%Y-%m-%d %H:%M:%S" || pattern == "%Y-%m-%dT%H:%M:%S") {
    const char sep = pattern == "%Y-%m-%dT%H:%M:%S" ? 'T' : ' ';
    if (text.size() != 19 || text[4] != '-' || text[7] != '-' || text[10] != sep || text[13] != ':' || text[16] != ':') {
      return std::nullopt;
    }
    if (!cast_parse_ndigits_(text, 0, 4, &y) ||
        !cast_parse_ndigits_(text, 5, 2, &mo) ||
        !cast_parse_ndigits_(text, 8, 2, &d) ||
        !cast_parse_ndigits_(text, 11, 2, &hh) ||
        !cast_parse_ndigits_(text, 14, 2, &mm) ||
        !cast_parse_ndigits_(text, 17, 2, &ss)) {
      return std::nullopt;
    }
    return build(y, mo, d, hh, mm, ss);
  }

  if (pattern == "%Y/%m/%d %H:%M:%S") {
    if (text.size() != 19 || text[4] != '/' || text[7] != '/' || text[10] != ' ' || text[13] != ':' || text[16] != ':') {
      return std::nullopt;
    }
    if (!cast_parse_ndigits_(text, 0, 4, &y) ||
        !cast_parse_ndigits_(text, 5, 2, &mo) ||
        !cast_parse_ndigits_(text, 8, 2, &d) ||
        !cast_parse_ndigits_(text, 11, 2, &hh) ||
        !cast_parse_ndigits_(text, 14, 2, &mm) ||
        !cast_parse_ndigits_(text, 17, 2, &ss)) {
      return std::nullopt;
    }
    return build(y, mo, d, hh, mm, ss);
  }

  return std::nullopt;
}

std::optional<std::int64_t> cast_timeline_tick_from_pattern_(
    std::string_view text,
    std::string_view pattern) {
  if (const auto fixed = cast_timeline_tick_from_fixed_pattern_(text, pattern); fixed.has_value()) {
    return fixed;
  }
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
    auto ticks = parsed.ticks();
    if (ticks.empty() || !parsed.isvalid().all().item<bool>()) {
      return std::nullopt;
    }
    return ticks.front();
  } catch (...) {
    return std::nullopt;
  }
}

Slate cast_note_to_timeline_(
    const Slate& source,
    const SlateCastOptions& options,
    std::string_view where) {
  const auto n = source.size();
  std::vector<std::int64_t> ticks(static_cast<std::size_t>(n), 0);
  std::vector<bool> valid = pluss::table::detail::has_validity_mask(source)
      ? pluss::table::detail::validity_mask(source)->host<bool>()
      : std::vector<bool>(static_cast<std::size_t>(n), true);
  const auto& notes = source.as_note();
  for (std::int64_t i = 0; i < n; ++i) {
    const auto pos = static_cast<std::size_t>(i);
    if (!valid[pos]) {
      valid[pos] = false;
      continue;
    }
    const auto text = notes.at(i);
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
  return Slate(out, bool_series_from_bools(valid));
}

std::optional<std::int64_t> cast_duration_nanos_from_text_(std::string_view text) {
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
    text.remove_prefix(1);
  }
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
    text.remove_suffix(1);
  }
  if (text.empty()) {
    return std::nullopt;
  }

  std::int64_t nanos = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), nanos);
  if (error != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return nanos;
}

} // namespace

SlateCastAccessor Slate::cast() const {
  return SlateCastAccessor(*this);
}

SlateCastAccessor Slate::cast(const SlateCastOptions& options) const {
  return SlateCastAccessor(*this, options);
}

SlateCastAccessor::SlateCastAccessor(Slate base, SlateCastOptions options)
    : base_(std::move(base)), options_(std::move(options)) {}

Slate SlateCastAccessor::series() const {
  return base_.cast_(SlateCastTarget::Series, options_);
}

Slate SlateCastAccessor::note() const {
  return base_.cast_(SlateCastTarget::Note, options_);
}

Slate SlateCastAccessor::timeline() const {
  return base_.cast_(SlateCastTarget::Timeline, options_);
}

Slate SlateCastAccessor::duration() const {
  return base_.cast_(SlateCastTarget::Duration, options_);
}

Slate Slate::cast_(SlateCastTarget target, std::optional<pluss::codem::ScalarType> series_dtype) const {
  return cast_(target, SlateCastOptions{.series_dtype = series_dtype});
}

Slate Slate::cast_(
    SlateCastTarget target,
    const SlateCastOptions& options) const {
  const auto series_dtype = options.series_dtype;
  switch (target) {
    case SlateCastTarget::Series: {
      if (kind() == SlateKind::Series) {
        if (!series_dtype.has_value() || as_series().scalar_type() == *series_dtype) {
          return *this;
        }
        return Slate(as_series().to(*series_dtype), validity_mask());
      }

      if (kind() == SlateKind::Timeline) {
        const auto ticks = as_timeline().ticks();
        auto out = pluss::Series::empty({static_cast<std::int64_t>(ticks.size())}, pluss::codem::ScalarType::Int64);
        if (!ticks.empty()) {
          std::memcpy(out.raw_data(), ticks.data(), ticks.size() * sizeof(std::int64_t));
        }
        if (series_dtype.has_value() && *series_dtype != pluss::codem::ScalarType::Int64) {
          out = out.to(*series_dtype);
        }
        return Slate(out, validity_mask());
      }

      if (kind() == SlateKind::Duration) {
        auto out = pluss::make_series(as_duration().host(), {size()},
                                      pluss::codem::ScalarType::Int64);
        if (series_dtype.has_value() && *series_dtype != pluss::codem::ScalarType::Int64) {
          out = out.to(*series_dtype);
        }
        return Slate(std::move(out), validity_mask());
      }

      PLUSS_CHECK(kind() == SlateKind::Note, "Slate::cast(Series): unsupported source type");
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
        return Slate(out, optional_mask_from_valid(parsed_valid));
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
      return Slate(out, optional_mask_from_valid(parsed_valid));
    }
    case SlateCastTarget::Note: {
      if (kind() == SlateKind::Note) {
        return *this;
      }
      if (kind() == SlateKind::Timeline) {
        return Slate(as_timeline().to_note_iso(), validity_mask());
      }
      if (kind() == SlateKind::Duration) {
        return Slate(as_duration().to_note_iso(), validity_mask());
      }

      PLUSS_CHECK(kind() == SlateKind::Series, "Slate::cast(Note): unsupported source type");
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(size()));
      for (std::int64_t i = 0; i < size(); ++i) {
        values.emplace_back(slate_scalar_string_at(*this, i));
      }
      return Slate(make_utf8_note(values), validity_mask());
    }
    case SlateCastTarget::Timeline: {
      if (kind() == SlateKind::Timeline) {
        return *this;
      }
      PLUSS_CHECK(kind() != SlateKind::Duration,
                  "Slate::cast(Timeline): Duration is elapsed time, not an epoch timestamp");
      if (kind() == SlateKind::Series) {
        PLUSS_CHECK(is_numeric_series_type(as_series()),
                    "Slate::cast(Timeline): Series source must be numeric epoch-nanos or YYYYMMDD");
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
              "Slate::cast(Timeline): invalid YYYYMMDD value(s)");
          return Slate(std::move(out), validity_mask());
        }

        std::vector<std::int64_t> ticks(static_cast<std::size_t>(size()), 0);
        for (std::int64_t i = 0; i < size(); ++i) {
          double v = 0.0;
          const bool ok = extract_numeric_value(as_series(), i, v);
          PLUSS_CHECK(ok, "Slate::cast(Timeline): failed numeric extraction from Series");
          ticks[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(v);
        }
        const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(ticks.size())};
        auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
            std::span<const std::int64_t>(ticks.data(), ticks.size()),
            std::span<const std::int64_t>(shape.data(), shape.size()));
        return Slate(out, validity_mask());
      }

      PLUSS_CHECK(kind() == SlateKind::Note, "Slate::cast(Timeline): unsupported source type");
      if (!options.format.empty() || options.invalid == ParseInvalid::Null) {
        return cast_note_to_timeline_(*this, options, "Slate::cast(Timeline)");
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
      return Slate(out, validity_mask());
    }
    case SlateCastTarget::Duration: {
      if (kind() == SlateKind::Duration) {
        return *this;
      }
      PLUSS_CHECK(options.format.empty(),
                  "Slate::cast(Duration): format is not accepted; text must contain exact signed nanoseconds");
      if (kind() == SlateKind::Series) {
        PLUSS_CHECK(as_series().scalar_type() == pluss::codem::ScalarType::Int64,
                    "Slate::cast(Duration): Series source must have Int64 dtype");
        return Slate(pluss::duration::nanos(as_series().pull<std::int64_t>(), {size()}),
                      validity_mask());
      }

      PLUSS_CHECK(kind() == SlateKind::Note,
                  "Slate::cast(Duration): source must be an Int64 Series or decimal-nanosecond Note");
      std::vector<std::int64_t> nanos(static_cast<std::size_t>(size()), 0);
      auto valid = has_validity_mask()
                       ? validity_mask()->host<bool>()
                       : std::vector<bool>(static_cast<std::size_t>(size()), true);
      for (std::int64_t i = 0; i < size(); ++i) {
        const auto pos = static_cast<std::size_t>(i);
        if (!valid[pos]) {
          continue;
        }
        const auto parsed = cast_duration_nanos_from_text_(as_note().at(i));
        if (parsed.has_value()) {
          nanos[pos] = *parsed;
          continue;
        }
        if (options.invalid == ParseInvalid::Null) {
          valid[pos] = false;
          continue;
        }
        PLUSS_FAIL("Slate::cast(Duration): could not parse exact signed nanoseconds from '" +
                   std::string(as_note().at(i)) + "'");
      }
      return Slate(pluss::duration::nanos(std::move(nanos), {size()}),
                    optional_mask_from_valid(valid));
    }
  }
  PLUSS_FAIL("Slate::cast: unsupported target");
}

Slate Slate::astype(pluss::codem::ScalarType dtype) const {
  return cast_(SlateCastTarget::Series, dtype);
}

TimeSeriesAccessor Slate::ts() const {
  return TimeSeriesAccessor(*this);
}

TimeSeriesAccessor::TimeSeriesAccessor(Slate base) : base_(std::move(base)) {}

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

TimeAxis ts_axis_to_index_(const Slate& axis, std::string_view where) {
  PLUSS_CHECK(axis.defined(), std::string(where) + ": time axis is undefined");
  PLUSS_CHECK(axis.kind() == SlateKind::Timeline,
              std::string(where) + ": time axis must be Timeline");
  return TimeAxis(axis);
}

pluss::Timeline::AsofDirection ts_asof_direction_(TsAlignMode mode) {
  switch (mode) {
    case TsAlignMode::Backward:
      return pluss::Timeline::AsofDirection::Backward;
    case TsAlignMode::Forward:
      return pluss::Timeline::AsofDirection::Forward;
    case TsAlignMode::Nearest:
      return pluss::Timeline::AsofDirection::Nearest;
    case TsAlignMode::Exact:
    default:
      return pluss::Timeline::AsofDirection::Backward;
  }
}

void ts_require_numeric_slate_(const Slate& value, std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": values are undefined");
  PLUSS_CHECK(value.kind() == SlateKind::Series && is_numeric_series_type(value.as_series()),
              std::string(where) + ": values must be numeric Series");
}

Slate ts_align_right_to_left_(const Slate& left,
                               const Slate& right,
                               const Slate& left_axis,
                               const Slate& right_axis,
                               TsAlignMode mode,
                               std::string_view where,
                               const pluss::Timeline::TimelineIndexAccessor* left_index = nullptr,
                               const pluss::Timeline::TimelineIndexAccessor* right_index = nullptr) {
  ts_require_numeric_slate_(left, where);
  ts_require_numeric_slate_(right, where);
  const auto left_axis_ta = ts_axis_to_index_(left_axis, where);
  const auto right_axis_ta = ts_axis_to_index_(right_axis, where);
  const auto n_left = left_axis_ta.size();
  const auto n_right = right_axis_ta.size();
  PLUSS_CHECK(left.size() == n_left,
              std::string(where) + ": left values and left time axis length mismatch");
  PLUSS_CHECK(right.size() == n_right,
              std::string(where) + ": right values and right time axis length mismatch");

  if (mode == TsAlignMode::Exact) {
    if (n_right == 0) {
      const auto platform = right.as_series().platform();
      return Slate(pluss::series::full({n_left},
                                std::numeric_limits<double>::quiet_NaN(),
                                pluss::Config{}.with_dtype(pluss::codem::ScalarType::Float64).with_platform(platform)),
                    pluss::Series::zeros({n_left}, pluss::codem::ScalarType::Bool, platform))
          .label().set(left_axis);
    }

    const auto left_ticks = left_axis_ta.timeline().epoch_nanos().host<std::int64_t>();
    const auto right_ticks = right_axis_ta.timeline().epoch_nanos().host<std::int64_t>();
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
        present.push_back(detail::valid_at(right, row));
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
    return Slate(values.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN()),
                  valid)
        .label().set(left_axis);
  }

  pluss::Series idx = pluss::series::undefined();
  if (n_right == 0) {
    idx = pluss::series::full({n_left}, -1.0, pluss::codem::ScalarType::Int64);
  } else {
    // join_asof's two-pointer merge assumes both sides are nondecreasing;
    // checked() validates that (throwing on an unsorted axis) instead of the
    // kernel silently producing wrong matches.
    const auto& ordered_left = left_index != nullptr ? *left_index : left_axis_ta.ordered();
    const auto& ordered_right = right_index != nullptr ? *right_index : right_axis_ta.ordered();
    idx = ordered_left.join_asof(ordered_right, ts_asof_direction_(mode), nullptr, true);
  }

  auto valid = idx.ge(0.0);
  if (n_left == 0 || n_right == 0) {
    const auto platform = right.as_series().platform();
    return Slate(pluss::series::full({n_left},
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
  auto out = Slate(gathered.masked_fill(out_valid.logical_not(), std::numeric_limits<double>::quiet_NaN()),
                    out_valid);
  return out.label().set(left_axis);
}

std::string grouped_window_output_name_(detail::TsGroupedWindowAggOp op,
                                        std::string_view value_feature) {
  switch (op) {
    case detail::TsGroupedWindowAggOp::Sum:
      return "sum_" + std::string(value_feature);
    case detail::TsGroupedWindowAggOp::Mean:
      return "mean_" + std::string(value_feature);
    case detail::TsGroupedWindowAggOp::Min:
      return "min_" + std::string(value_feature);
    case detail::TsGroupedWindowAggOp::Max:
      return "max_" + std::string(value_feature);
    case detail::TsGroupedWindowAggOp::Count:
      return "count_" + std::string(value_feature);
  }
  PLUSS_FAIL("detail::grouped_window_aggregate: unsupported grouped window aggregate");
}

DataFrame::GroupOp grouped_window_group_op_(detail::TsGroupedWindowAggOp op) {
  switch (op) {
    case detail::TsGroupedWindowAggOp::Sum:
      return DataFrame::GroupOp::Sum;
    case detail::TsGroupedWindowAggOp::Mean:
      return DataFrame::GroupOp::Mean;
    case detail::TsGroupedWindowAggOp::Min:
      return DataFrame::GroupOp::Min;
    case detail::TsGroupedWindowAggOp::Max:
      return DataFrame::GroupOp::Max;
    case detail::TsGroupedWindowAggOp::Count:
      return DataFrame::GroupOp::Count;
  }
  PLUSS_FAIL("detail::grouped_window_aggregate: unsupported grouped window aggregate");
}

std::vector<std::string> default_group_names_(std::size_t count) {
  std::vector<std::string> out;
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back(count == 1 ? "group" : "group_" + std::to_string(i));
  }
  return out;
}

std::vector<std::string_view> grouped_window_key_views_(const std::vector<std::string>& keys) {
  std::vector<std::string_view> out;
  out.reserve(keys.size());
  for (const auto& key : keys) {
    out.push_back(key);
  }
  return out;
}

Slate grouped_window_axis_slate_(const Slate& axis,
                                   std::string_view where) {
  PLUSS_CHECK(axis.defined(),
              std::string(where) + ": time axis is undefined");
  if (axis.kind() == SlateKind::Timeline) {
    return axis.tl().epoch(TimeUnit::Nanos).cast({.series_dtype = ScalarType::Float64}).series();
  }
  PLUSS_CHECK(axis.kind() == SlateKind::Series &&
                  axis.as_series().scalar_type() == ScalarType::Int64,
              std::string(where) +
                  ": time axis must be Timeline or Int64 epoch-nanos Series");
  return axis.cast({.series_dtype = ScalarType::Float64}).series();
}

void set_grouped_window_axis_(DataFrame& frame,
                              const Slate& axis,
                              std::string_view output_feature,
                              std::string_view where) {
  PLUSS_CHECK(axis.size() == frame.ncases(),
              std::string(where) + ": time axis length must match DataFrame cases");
  frame.set_(output_feature,
             grouped_window_axis_slate_(axis, where));
}

Slate grouped_window_mask_(const Slate& axis,
                            const Slate& anchor,
                            std::int64_t duration_nanos,
                            WindowClosure closure) {
  const auto lower = anchor.sub(static_cast<double>(duration_nanos));
  switch (closure) {
    case WindowClosure::Right:
      return axis.gt(lower).land(axis.le(anchor));
    case WindowClosure::Left:
      return axis.ge(lower).land(axis.lt(anchor));
    case WindowClosure::Both:
      return axis.ge(lower).land(axis.le(anchor));
    case WindowClosure::Neither:
      return axis.gt(lower).land(axis.lt(anchor));
  }
  PLUSS_FAIL("detail::grouped_window_aggregate: unsupported closure");
}

} // namespace

namespace detail {

DataFrame grouped_window_aggregate(const DataFrame& frame,
                                   const Slate& axis,
                                   const TsGroup& groups,
                                   const Slate* anchor,
                                   std::int64_t duration_nanos,
                                   WindowClosure closure,
                                   std::string_view value_feature,
                                   TsGroupedWindowAggOp op,
                                   std::string_view where) {
  PLUSS_CHECK(groups.defined(),
              std::string(where) + ": group keys are required");
  PLUSS_CHECK(groups.size() == frame.ncases(),
              std::string(where) + ": group length must match DataFrame cases");

  auto public_names = groups.names;
  if (public_names.empty()) {
    public_names = default_group_names_(groups.keys.size());
  }
  PLUSS_CHECK(public_names.size() == groups.keys.size(),
              std::string(where) + ": group names length must match group key count");

  auto working = frame;
  std::vector<std::string> internal_names;
  internal_names.reserve(groups.keys.size());
  for (std::size_t i = 0; i < groups.keys.size(); ++i) {
    const auto internal = "__ts_group_" + std::to_string(i) + "__";
    working.set_(internal, groups.keys[i]);
    internal_names.push_back(internal);
  }
  const auto key_views = grouped_window_key_views_(internal_names);

  constexpr std::string_view time_axis_name = "__ts_window_time__";
  constexpr std::string_view anchor_axis_name = "__ts_window_anchor__";
  set_grouped_window_axis_(working, axis, time_axis_name, where);
  if (anchor != nullptr && anchor->defined()) {
    set_grouped_window_axis_(working, *anchor, anchor_axis_name, where);
  } else {
    const auto anchors = working.group({.dropnull = true})
                             .by(pluss::msharp::RangeView::Of<std::string_view>(key_views))
                             .agg({DataFrame::GroupSpec{
                                 std::string(anchor_axis_name),
                                 std::string(time_axis_name),
                                 DataFrame::GroupOp::Max,
                             }});
    working = working.unify(pluss::msharp::RangeView::Of<std::string_view>(key_views),
                            anchors,
                            pluss::msharp::RangeView::Of<std::string_view>(key_views),
                            "keep_left",
                            "_anchor",
                            "many_to_one");
  }

  const auto mask = grouped_window_mask_(working.features(std::string(time_axis_name)),
                                         working.features(std::string(anchor_axis_name)),
                                         duration_nanos,
                                         closure);
  const auto filtered = working.filter(mask);
  DataFrame out = filtered.group({.dropnull = true})
                      .by(pluss::msharp::RangeView::Of<std::string_view>(key_views))
                      .agg({DataFrame::GroupSpec{
                          grouped_window_output_name_(op, value_feature),
                          std::string(value_feature),
                          grouped_window_group_op_(op),
                      }});
  for (std::size_t i = 0; i < internal_names.size(); ++i) {
    if (internal_names[i] != public_names[i]) {
      out.rename_(internal_names[i], public_names[i]);
    }
  }
  return out;
}

} // namespace detail

const pluss::Series& TimeSeriesAccessor::series_payload_(const char* op) const {
  PLUSS_CHECK(base_.defined(), std::string("TimeSeriesAccessor::") + op + ": source Slate is undefined");
  PLUSS_CHECK(base_.kind() == SlateKind::Series, std::string("TimeSeriesAccessor::") + op + ": source must be Series");
  PLUSS_CHECK(is_numeric_series_type(base_.as_series()),
              std::string("TimeSeriesAccessor::") + op + ": source Series must be numeric");
  return base_.as_series();
}

// series_payload_() hands back the raw underlying Series, with no knowledge of
// base_'s own boolean validity mask. Methods that delegate straight to the
// pluss::subsp::stats::ts:: free functions (which only ever see a bare Series,
// never a Slate) would otherwise silently treat an explicitly-masked-missing
// entry as real, valid data whenever its raw storage isn't itself NaN. This
// helper closes that gap: it bakes base_'s validity mask into the returned
// Series as real NaN at every invalid position (promoting to a floating dtype
// first if needed), so downstream missingness-propagation in those free
// functions — and the plain Slate(Series) that wraps their result — sees
// exactly the same missing positions the source Slate does.
pluss::Series TimeSeriesAccessor::masked_series_payload_(const char* op) const {
  const auto& raw = series_payload_(op);
  if (!detail::has_validity_mask(base_)) {
    return raw;
  }
  const auto real_dtype = raw.dtype().is_floating()
                               ? raw.scalar_type()
                               : (raw.platform() == pluss::codem::Platform::MPS
                                      ? pluss::codem::ScalarType::Float32
                                      : pluss::codem::ScalarType::Float64);
  auto real = raw.to(real_dtype);
  auto mask = *detail::validity_mask(base_);
  if (mask.platform() != real.platform()) {
    mask = mask.to(real.platform());
  }
  return real.masked_fill(mask.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

Slate TimeSeriesAccessor::returns(std::int64_t periods) const {
  return Slate(pluss::subsp::stats::ts::returns(masked_series_payload_("returns"), periods));
}

Slate TimeSeriesAccessor::log_returns(std::int64_t periods) const {
  return Slate(pluss::subsp::stats::ts::log_returns(masked_series_payload_("log_returns"), periods));
}

Slate TimeSeriesAccessor::diff(std::int64_t periods) const {
  return Slate(pluss::subsp::stats::ts::diff(masked_series_payload_("diff"), periods));
}

Slate TimeSeriesAccessor::lag(std::int64_t periods) const {
  return Slate(pluss::subsp::stats::ts::lag(masked_series_payload_("lag"), periods));
}

Slate TimeSeriesAccessor::lead(std::int64_t periods) const {
  return Slate(pluss::subsp::stats::ts::lead(masked_series_payload_("lead"), periods));
}

Slate TimeSeriesAccessor::rolling(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  (void)series_payload_("rolling");
  return base_.moving(window, statistic, options);
}

Slate TimeSeriesAccessor::rolling(
    const Slate& axis,
    std::string_view duration,
    WindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  (void)series_payload_("rolling");
  PLUSS_CHECK(axis.kind() == SlateKind::Timeline,
              "TimeSeriesAccessor::rolling: axis must be Timeline");
  return base_.timed(axis,
                     ts_duration_nanos_(duration, "TimeSeriesAccessor::rolling"),
                     to_time_window_stat_(statistic, "TimeSeriesAccessor::rolling"),
                     options,
                     closure);
}

Slate TimeSeriesAccessor::rolling(
    const Slate& axis,
    const Slate& groups,
    std::string_view duration,
    WindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  (void)series_payload_("rolling");
  PLUSS_CHECK(axis.kind() == SlateKind::Timeline,
              "TimeSeriesAccessor::rolling: axis must be Timeline");
  PLUSS_CHECK(groups.defined(),
              "TimeSeriesAccessor::rolling: groups are undefined");
  PLUSS_CHECK(groups.size() == base_.size(),
              "TimeSeriesAccessor::rolling: groups length must match values");

  Slate cpu_groups = groups;
  if (groups.kind() == SlateKind::Series &&
      groups.as_series().platform() != pluss::codem::Platform::CPU) {
    cpu_groups = Slate(
        groups.as_series().to(pluss::codem::Platform::CPU),
        groups.validity_mask());
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_group;
  rows_by_group.reserve(static_cast<std::size_t>(base_.size()));
  for (std::int64_t row = 0; row < base_.size(); ++row) {
    const auto key =
        groups.is_valid(row)
            ? std::string("\1") + slate_scalar_string_at(cpu_groups, row)
            : std::string(1, '\0');
    rows_by_group[key].push_back(row);
  }

  const auto platform = base_.as_series().platform();
  const auto dtype = platform == pluss::codem::Platform::CPU
                         ? pluss::codem::ScalarType::Float64
                         : pluss::codem::ScalarType::Float32;
  auto output = pluss::Series::zeros({base_.size()}, dtype, platform);
  auto output_valid = pluss::Series::zeros(
      {base_.size()}, pluss::codem::ScalarType::Bool, platform);

  for (const auto& [key, rows] : rows_by_group) {
    (void)key;
    const auto index = pluss::make_series(
        rows,
        {static_cast<std::int64_t>(rows.size())},
        Config{}.with_dtype(pluss::codem::ScalarType::Int64).with_platform(platform));
    const auto result = base_.take(rows).ts().rolling(
        axis.take(rows), duration, statistic, options, closure);
    output = output.index_copy(0, index, result.as_series().to(dtype));
    output_valid = output_valid.index_copy(
        0, index, result.notnull().as_series().to(platform));
  }

  return Slate(output, output_valid).label().set(axis);
}

Slate TimeSeriesAccessor::timed(
    const Slate& axis,
    std::int64_t duration_nanos,
    TimeWindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  (void)series_payload_("timed");
  return base_.timed(axis, duration_nanos, statistic, options, closure);
}

Slate TimeSeriesAccessor::grouped(
    const Slate& groups,
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  (void)series_payload_("grouped");
  return base_.grouped(groups, window, statistic, options);
}

Slate TimeSeriesAccessor::expanding(
    WindowStatistic statistic,
    const StatisticOptions& options) const {
  (void)series_payload_("expanding");
  return base_.growing(statistic, options);
}

Slate TimeSeriesAccessor::ewm(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  (void)series_payload_("ewm");
  return base_.decay(alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls);
}

Slate TimeSeriesAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods, double epsilon) const {
  return Slate(pluss::subsp::stats::ts::zscore_rolling(
      masked_series_payload_("zscore_rolling"), window, min_periods, epsilon));
}

Slate TimeSeriesAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode, std::optional<double> constant) const {
  return Slate(pluss::subsp::stats::ts::fill_gaps(masked_series_payload_("fill_gaps"), mode, constant));
}

Slate TimeSeriesAccessor::sort(const Slate& axis,
                                bool ascending,
                                bool nulls_last) const {
  (void)series_payload_("sort");
  PLUSS_CHECK(axis.size() == base_.size(),
              "TimeSeriesAccessor::sort: axis length must match values");
  PLUSS_CHECK(axis.kind() == SlateKind::Timeline,
              "TimeSeriesAccessor::sort: axis must be Timeline");
  return base_.label().set(axis).label().sort(ascending, nulls_last);
}

Slate TimeSeriesAccessor::align_to(const Slate& source_axis,
                                    const Slate& target_axis,
                                    TsAlignMode mode) const {
  const auto& source = series_payload_("align_to");
  const auto target_axis_ta = ts_axis_to_index_(target_axis, "TimeSeriesAccessor::align_to");
  const auto platform = source.platform();
  auto target_values = Slate(
      pluss::Series::zeros({target_axis_ta.size()}, pluss::codem::ScalarType::Float64, platform));
  return ts_align_right_to_left_(
      target_values, base_, target_axis, source_axis, mode, "TimeSeriesAccessor::align_to");
}

Slate TimeSeriesAccessor::align_index_(
    const Slate& other,
    const Slate& left_axis,
    const Slate& right_axis,
    TsAlignMode mode,
    const pluss::Timeline::TimelineIndexAccessor* left_index,
    const pluss::Timeline::TimelineIndexAccessor* right_index) const {
  return ts_align_right_to_left_(base_, other, left_axis, right_axis, mode,
                                 "DataFrameTsAccessor::binary", left_index,
                                 right_index);
}

Slate TimeSeriesAccessor::align_to(const Slate& source_axis,
                                    const Slate& target_axis,
                                    TsAlignMode mode,
                                    pluss::subsp::stats::ts::GapFillMode fill_mode,
                                    std::optional<double> constant) const {
  return align_to(source_axis, target_axis, mode).ts().fill_gaps(fill_mode, constant);
}

DataFrame TimeSeriesAccessor::lag_block(pluss::msharp::RangeView::Int lags) const {
  PLUSS_CHECK(lags.size() > 0, "TimeSeriesAccessor::lag_block: expected at least one lag");
  const auto series = masked_series_payload_("lag_block");
  std::vector<std::pair<std::string, Slate>> out;
  out.reserve(lags.size());
  for (const auto lag : lags) {
    out.emplace_back("lag" + std::to_string(lag), Slate(pluss::subsp::stats::ts::lag(series, lag)));
  }
  return DataFrame(std::move(out), base_.caselbl());
}

DataFrame TimeSeriesAccessor::rolling_block(pluss::msharp::RangeView::Int windows,
                                            const TsRollingBlockOptions& options) const {
  PLUSS_CHECK(windows.size() > 0, "TimeSeriesAccessor::rolling_block: expected at least one window");
  const auto series = masked_series_payload_("rolling_block");
  std::vector<std::pair<std::string, Slate>> out;
  for (const auto w : windows) {
    if (options.include_mean) {
      out.emplace_back("roll_mean_w" + std::to_string(w),
                       Slate(pluss::subsp::stats::ts::rolling_mean(series, w, options.min_periods)));
    }
    if (options.include_std) {
      out.emplace_back("roll_std_w" + std::to_string(w),
                       Slate(pluss::subsp::stats::ts::rolling_std(series, w, options.min_periods)));
    }
    if (options.include_min) {
      out.emplace_back("roll_min_w" + std::to_string(w),
                       Slate(pluss::subsp::stats::ts::rolling_min(series, w, options.min_periods)));
    }
    if (options.include_max) {
      out.emplace_back("roll_max_w" + std::to_string(w),
                       Slate(pluss::subsp::stats::ts::rolling_max(series, w, options.min_periods)));
    }
    if (options.include_sum) {
      out.emplace_back("roll_sum_w" + std::to_string(w),
                       Slate(pluss::subsp::stats::ts::rolling_sum(series, w, options.min_periods)));
    }
    if (options.include_var) {
      out.emplace_back("roll_var_w" + std::to_string(w),
                       Slate(pluss::subsp::stats::ts::rolling_var(series, w, options.min_periods)));
    }
  }
  return DataFrame(std::move(out), base_.caselbl());
}

TimeSeriesResampler TimeSeriesAccessor::resample(const Slate& axis,
                                                 std::string_view freq,
                                                 std::int64_t step) const {
  return TimeSeriesResampler(
      base_,
      axis,
      pluss::Timeline::ResampleOptions{.freq = std::string(freq), .step = step});
}

TimeSeriesResampler TimeSeriesAccessor::resample(
    const Slate& axis,
    const pluss::Timeline::ResampleOptions& options) const {
  (void)series_payload_("resample");
  return TimeSeriesResampler(base_, axis, options);
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

TimeSeriesResampler::TimeSeriesResampler(Slate base,
                                         Slate axis,
                                         pluss::Timeline::ResampleOptions options)
    : base_(std::move(base)),
      axis_(std::move(axis)),
      options_(std::move(options)) {
  PLUSS_CHECK(base_.defined(), "TimeSeriesResampler: source Slate is undefined");
  PLUSS_CHECK(base_.kind() == SlateKind::Series && is_numeric_series_type(base_.as_series()),
              "TimeSeriesResampler: source Slate must be a numeric Series");
  PLUSS_CHECK(axis_.defined(), "TimeSeriesResampler: axis Slate is undefined");
  PLUSS_CHECK(axis_.kind() == SlateKind::Timeline,
              "TimeSeriesResampler: axis must be Timeline");
  PLUSS_CHECK(axis_.size() == base_.size(),
              "TimeSeriesResampler: axis length must match source Slate");
  PLUSS_CHECK(!options_.freq.empty(), "TimeSeriesResampler: freq cannot be empty");
  PLUSS_CHECK(options_.step > 0, "TimeSeriesResampler: step must be positive");
}

Slate TimeSeriesResampler::aggregate_(pluss::subsp::stats::ts::ResampleOp op) const {
  auto resampled = base_.ts().resample(axis_, options_, op);
  return resampled.values.label().set(resampled.labels);
}

Slate TimeSeriesResampler::mean() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Mean);
}

Slate TimeSeriesResampler::sum() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Sum);
}

Slate TimeSeriesResampler::min() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Min);
}

Slate TimeSeriesResampler::max() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Max);
}

Slate TimeSeriesResampler::first() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::First);
}

Slate TimeSeriesResampler::last() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Last);
}

Slate TimeSeriesResampler::count() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Count);
}

Slate TimeSeriesResampler::asfreq(std::optional<double> fill_value) const {
  const auto axis_ta = ts_axis_to_index_(axis_, "TimeSeriesResampler::asfreq");
  const auto target_axis = Slate(axis_ta.ordered().resample_bins(options_).labels);
  auto out = base_.ts().align_to(axis_, target_axis, TsAlignMode::Exact);
  if (!fill_value.has_value()) {
    return out;
  }
  const auto target_axis_ta = ts_axis_to_index_(target_axis, "TimeSeriesResampler::asfreq");
  const auto n_source = axis_ta.size();
  const auto n_target = target_axis_ta.size();
  Series idx = pluss::series::full({n_target}, -1.0, ScalarType::Int64);
  if (n_source > 0 && n_target > 0) {
    // join_asof's two-pointer merge assumes nondecreasing ticks on both
    // sides; checked() validates that instead of trusting it silently.
    idx = target_axis_ta.ordered().join_asof(axis_ta.ordered(), pluss::Timeline::AsofDirection::Backward, nullptr, true);
  }
  auto exact = pluss::Series::zeros({n_target}, pluss::codem::ScalarType::Bool);
  if (n_source > 0 && n_target > 0) {
    const Series valid = idx.ge(0.0);
    const Series source_ticks = axis_ta.timeline().epoch_nanos().reshape({n_source});
    const Series target_ticks = target_axis_ta.timeline().epoch_nanos().reshape({n_target});
    const Series safe_idx = idx.to(ScalarType::Float64).clamp(0.0, static_cast<double>(n_source - 1)).to(ScalarType::Int64);
    const Series gathered_ticks = source_ticks.index_select(0, safe_idx);
    exact = valid.logical_and(gathered_ticks.eq(target_ticks));
  }
  return Slate(out.as_series().where(exact.logical_not(), *fill_value)).label().set(target_axis);
}

Slate TimeSeriesResampler::ffill() const {
  const auto target_axis = Slate(ts_axis_to_index_(axis_, "TimeSeriesResampler::ffill").ordered().resample_bins(options_).labels);
  return base_.ts().align_to(axis_, target_axis, TsAlignMode::Backward);
}

Slate TimeSeriesResampler::bfill() const {
  const auto target_axis = Slate(ts_axis_to_index_(axis_, "TimeSeriesResampler::bfill").ordered().resample_bins(options_).labels);
  return base_.ts().align_to(axis_, target_axis, TsAlignMode::Forward);
}

Slate TimeSeriesResampler::nearest() const {
  const auto target_axis = Slate(ts_axis_to_index_(axis_, "TimeSeriesResampler::nearest").ordered().resample_bins(options_).labels);
  return base_.ts().align_to(axis_, target_axis, TsAlignMode::Nearest);
}

TimeSeriesResample TimeSeriesAccessor::resample(const Slate& axis,
                                                std::string_view freq,
                                                pluss::subsp::stats::ts::ResampleOp op,
                                                std::int64_t step) const {
  return resample(axis,
                  pluss::Timeline::ResampleOptions{.freq = std::string(freq), .step = step},
                  op);
}

TimeSeriesResample TimeSeriesAccessor::resample(
    const Slate& axis,
    const pluss::Timeline::ResampleOptions& options,
    pluss::subsp::stats::ts::ResampleOp op) const {
  PLUSS_CHECK(axis.defined(), "TimeSeriesAccessor::resample: axis Slate is undefined");
  PLUSS_CHECK(axis.kind() == SlateKind::Timeline,
              "TimeSeriesAccessor::resample: axis must be Timeline");
  const auto axis_ta = TimeAxis(axis);
  return resample_index_(axis_ta.ordered(), options, op);
}

TimeSeriesResample TimeSeriesAccessor::resample_index_(
    const pluss::Timeline::TimelineIndexAccessor& time_index,
    const pluss::Timeline::ResampleOptions& options,
    pluss::subsp::stats::ts::ResampleOp op) const {
  const auto bins = time_index.resample_bins(options);
  const auto values =
      op == pluss::subsp::stats::ts::ResampleOp::Count
          ? time_index.resample_sum(base_.notnull().as_series().to(pluss::codem::ScalarType::Int64), options)
          : time_index.resample_agg(
                masked_series_payload_("resample"),
                options,
                time_series_resample_op_key_(op));
  return TimeSeriesResample{
      .labels = Slate(bins.labels),
      .values = Slate(values),
  };
}

Slate TimeSeriesAccessor::add(const Slate& other,
                               const Slate& left_axis,
                               const Slate& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("add");
  return base_.add(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::add"));
}

Slate TimeSeriesAccessor::sub(const Slate& other,
                               const Slate& left_axis,
                               const Slate& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("sub");
  return base_.sub(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::sub"));
}

Slate TimeSeriesAccessor::mul(const Slate& other,
                               const Slate& left_axis,
                               const Slate& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("mul");
  return base_.mul(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::mul"));
}

Slate TimeSeriesAccessor::div(const Slate& other,
                               const Slate& left_axis,
                               const Slate& right_axis,
                               TsAlignMode mode) const {
  (void)series_payload_("div");
  return base_.div(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::div"));
}

Slate TimeSeriesAccessor::eq(const Slate& other,
                              const Slate& left_axis,
                              const Slate& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("eq");
  return base_.eq(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::eq"));
}

Slate TimeSeriesAccessor::ne(const Slate& other,
                              const Slate& left_axis,
                              const Slate& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("ne");
  return base_.ne(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::ne"));
}

Slate TimeSeriesAccessor::lt(const Slate& other,
                              const Slate& left_axis,
                              const Slate& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("lt");
  return base_.lt(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::lt"));
}

Slate TimeSeriesAccessor::le(const Slate& other,
                              const Slate& left_axis,
                              const Slate& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("le");
  return base_.le(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::le"));
}

Slate TimeSeriesAccessor::gt(const Slate& other,
                              const Slate& left_axis,
                              const Slate& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("gt");
  return base_.gt(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::gt"));
}

Slate TimeSeriesAccessor::ge(const Slate& other,
                              const Slate& left_axis,
                              const Slate& right_axis,
                              TsAlignMode mode) const {
  (void)series_payload_("ge");
  return base_.ge(ts_align_right_to_left_(base_, other, left_axis, right_axis, mode, "TimeSeriesAccessor::ge"));
}

DataFrame TimeSeriesAccessor::calendar(const Slate& axis) const {
  PLUSS_CHECK(axis.defined(), "TimeSeriesAccessor::calendar: axis Slate is undefined");
  PLUSS_CHECK(axis.kind() == SlateKind::Timeline,
              "TimeSeriesAccessor::calendar: axis must be Timeline");
  std::vector<std::pair<std::string, Slate>> out;
  out.reserve(5);
  out.emplace_back("axis_year", axis.year());
  out.emplace_back("axis_month", axis.month());
  out.emplace_back("axis_day", axis.day());
  out.emplace_back("axis_hour", axis.hour());
  out.emplace_back("axis_weekday", axis.tl().weekday_name());
  return DataFrame(std::move(out), axis);
}
