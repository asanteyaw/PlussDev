Stream::Stream(pluss::msharp::intrusive_ptr<detail::StreamImpl> impl) : impl_(std::move(impl)) {}

Stream::Stream() = default;

Stream::Stream(const pluss::Series& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const pluss::Note& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const pluss::Timeline& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const pluss::TimeIndex& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const Stream& other) = default;
Stream::Stream(Stream&& other) noexcept = default;
Stream& Stream::operator=(const Stream& other) = default;
Stream& Stream::operator=(Stream&& other) noexcept = default;
Stream::~Stream() = default;

Stream Stream::from_thread(const pluss::table::Thread& thread, std::optional<pluss::table::Thread> cases) {
  PLUSS_CHECK(thread.defined(), "Stream::from_thread requires a defined thread");
  switch (thread.kind()) {
    case pluss::table::ThreadKind::Series:
      return Stream(thread.as_series(), pluss::table::detail::validity_mask(thread), std::move(cases));
    case pluss::table::ThreadKind::Note:
      return Stream(thread.as_note(), pluss::table::detail::validity_mask(thread), std::move(cases));
    case pluss::table::ThreadKind::Timeline:
      return Stream(thread.as_timeline(), pluss::table::detail::validity_mask(thread), std::move(cases));
    case pluss::table::ThreadKind::TimeIndex:
      return Stream(thread.as_timeindex(), pluss::table::detail::validity_mask(thread), std::move(cases));
  }
  pluss::codem::throw_error("Stream::from_thread: unsupported thread kind");
}

bool Stream::defined() const noexcept { return static_cast<bool>(impl_); }
bool Stream::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Stream::size() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().size_;
}

std::int64_t Stream::nrecords() const {
  return size();
}

bool Stream::empty() const {
  return size() == 0;
}

std::pair<std::int64_t, std::int64_t> Stream::shape() const {
  return {size(), 1};
}

StreamKind Stream::kind() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().kind_;
}

std::string Stream::ptype() const {
  switch (kind()) {
    case StreamKind::Series:
      return "Series";
    case StreamKind::Note:
      return "Note";
    case StreamKind::Timeline:
      return "Timeline";
    case StreamKind::TimeIndex:
      return "TimeIndex";
  }
  PLUSS_FAIL("Stream::ptype: unknown StreamKind");
}

std::string Stream::dtype() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().dtype_;
}

Stream Stream::isnull() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().isnull(), copy_label(*this));
}

Stream Stream::notnull() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().notnull(), copy_label(*this));
}

Stream Stream::invalid() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().invalid(), copy_label(*this));
}

Stream Stream::fillnull(double value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Thread& t) { return t.fillnull(value); });
}

Stream Stream::fillnull(std::string_view value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Thread& t) { return t.fillnull(value); });
}

Stream Stream::fillnull(const pluss::Timeline& value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Thread& t) { return t.fillnull(value); });
}

Stream Stream::fillnull(const pluss::TimeIndex& value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Thread& t) { return t.fillnull(value); });
}

Stream Stream::abs() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Thread& t) { return t.abs(); });
}

Stream Stream::clip(double min_value, double max_value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Thread& t) { return t.clip(min_value, max_value); });
}

Stream Stream::log() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Thread& t) { return t.log(); });
}

Stream Stream::exp() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Thread& t) { return t.exp(); });
}

bool Stream::has_validity_mask() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().validity_mask_.has_value();
}

const std::optional<pluss::Series>& Stream::validity_mask() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().validity_mask_;
}

Stream Stream::with_validity_mask(const pluss::Series& mask) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_validity_mask_(mask);
  return out;
}

Stream& Stream::with_validity_mask_(const pluss::Series& mask) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  validate_mask_size(mask, w.size_);
  w.validity_mask_ = mask;
  w.sync_fields();
  return *this;
}

Stream Stream::clear_validity_mask() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.clear_validity_mask_();
  return out;
}

Stream& Stream::clear_validity_mask_() {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  w.sync_fields();
  return *this;
}

bool Stream::has_label() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().cases_.has_value();
}

const std::optional<pluss::table::Thread>& Stream::label() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().cases_;
}

Stream Stream::with_label(const pluss::table::Thread& label) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_label_(label);
  return out;
}

Stream& Stream::with_label_(const pluss::table::Thread& label) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  validate_label_size(label, w.size_);
  w.cases_ = label;
  w.sync_fields();
  return *this;
}

Stream Stream::clear_label() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.clear_label_();
  return out;
}

Stream& Stream::clear_label_() {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  w.cases_.reset();
  w.sync_fields();
  return *this;
}

bool Stream::has_meta(std::string_view key) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().meta_.find(std::string(key)) != impl_ref_().meta_.end();
}

std::optional<std::string> Stream::meta(std::string_view key) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto it = impl_ref_().meta_.find(std::string(key));
  if (it == impl_ref_().meta_.end()) return std::nullopt;
  return it->second;
}

const std::unordered_map<std::string, std::string>& Stream::meta() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().meta_;
}

Stream Stream::with_meta(std::string key, std::string value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_meta_(std::move(key), std::move(value));
  return out;
}

Stream& Stream::with_meta_(std::string key, std::string value) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  PLUSS_CHECK(!key.empty(), "Stream::with_meta_: key cannot be empty");
  mut_impl_().meta_[std::move(key)] = std::move(value);
  return *this;
}

Stream Stream::with_meta(std::initializer_list<std::pair<std::string_view, std::string_view>> items) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_meta_(items);
  return out;
}

Stream& Stream::with_meta_(std::initializer_list<std::pair<std::string_view, std::string_view>> items) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  for (const auto& [k, v] : items) {
    with_meta_(std::string(k), std::string(v));
  }
  return *this;
}

Stream Stream::drop_meta(std::string_view key) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.drop_meta_(key);
  return out;
}

Stream& Stream::drop_meta_(std::string_view key) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  mut_impl_().meta_.erase(std::string(key));
  return *this;
}
