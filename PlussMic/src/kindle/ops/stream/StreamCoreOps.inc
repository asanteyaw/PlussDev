pluss::table::Thread Stream::to_thread() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return std::visit(
      [&](const auto& payload) {
        return pluss::table::Thread(payload, impl_ref_().validity_mask_);
      },
      impl_ref_().payload_);
}

pluss::table::Thread Stream::to_thread(std::int64_t start, std::int64_t stop) const {
  const auto t = to_thread();
  return t.slice(start, stop);
}

Stream Stream::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.head(n); });
}

Stream Stream::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.tail(n); });
}

Stream Stream::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.slice(start, stop); });
}

Stream Stream::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.take(indices); });
}

Stream Stream::filter(const pluss::Series& bool_mask) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.filter(bool_mask); });
}

Stream Stream::dropna() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto source = to_thread();
  const auto out = source.dropnull();
  if (!has_cases()) return Stream::from_thread(out);
  const auto cases_out = cases()->filter(source.notnull().as_series());
  return Stream::from_thread(out, cases_out);
}

Stream Stream::ffill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.ffill(); });
}

Stream Stream::bfill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.bfill(); });
}

Stream Stream::cast(pluss::table::ThreadCastTarget target,
                    std::optional<pluss::codem::ScalarType> series_dtype) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [&](const pluss::table::Thread& t) { return t.cast(target, series_dtype); });
}

Stream Stream::astype(pluss::codem::ScalarType dtype) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [&](const pluss::table::Thread& t) { return t.astype(dtype); });
}

Stream Stream::sum() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().sum().thread());
}

Stream Stream::mean() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().mean().thread());
}

Stream Stream::min() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().min().thread());
}

Stream Stream::max() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().max().thread());
}

Stream Stream::var(bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().var(unbiased).thread());
}

Stream Stream::std(bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().std(unbiased).thread());
}

const pluss::Series& Stream::as_series() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Series>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Series");
  return *p;
}

const pluss::Note& Stream::as_note() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Note>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Note");
  return *p;
}

const pluss::Timeline& Stream::as_timeline() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Timeline>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Timeline");
  return *p;
}

const pluss::TimeIndex& Stream::as_timeindex() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::TimeIndex>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not TimeIndex");
  return *p;
}

std::string Stream::to_string() const {
  return display::render(*this);
}

std::string Stream::to_json() const {
  return display::render(*this);
}

void Stream::show(std::ostream& os) const { display::print(os, *this); }

detail::StreamImpl& Stream::mut_impl_() {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  if (impl_.use_count() > 1) {
    auto cloned = pluss::msharp::make_intrusive<detail::StreamImpl>(
        impl_ref_().payload_, impl_ref_().validity_mask_, impl_ref_().cases_);
    cloned->meta_ = impl_ref_().meta_;
    cloned->sync_fields();
    impl_ = std::move(cloned);
  }
  return *impl_;
}

const detail::StreamImpl& Stream::impl_ref_() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Stream& stream) {
  stream.show(os);
  return os;
}
