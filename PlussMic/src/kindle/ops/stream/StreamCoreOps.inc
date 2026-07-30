pluss::table::Thread Stream::to_thread() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = std::visit(
      [&](const auto& payload) {
        return pluss::table::Thread(payload, impl_ref_().validity_mask_);
      },
      impl_ref_().payload_);
  if (has_label()) {
    out = out.label().set(*label());
  }
  return out;
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

Stream Stream::dropnull() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto source = to_thread();
  const auto out = source.dropnull();
  if (!has_label()) return Stream::from_thread(out);
  const auto label_out = label()->filter(source.notnull().as_series());
  return Stream::from_thread(out, label_out);
}

Stream Stream::ffill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Thread& t) { return t.ffill(); });
}

Stream Stream::bfill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Thread& t) { return t.bfill(); });
}

Stream Stream::distinct(DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto out = to_thread().distinct(options.dropnull);
  if (out.label().has()) {
    return Stream::from_thread(out, out.label().values());
  }
  return Stream::from_thread(out);
}

Stream Stream::freq(FreqOptions options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto out = pluss::table::detail::freq_count_thread(
      to_thread(), options.top, options.dropnull, "freq");
  if (out.label().has()) {
    return Stream::from_thread(out, out.label().values());
  }
  return Stream::from_thread(out);
}

Stream Stream::ndistinct(DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(to_thread().ndistinct(options.dropnull).thread());
}

Stream Stream::astype(pluss::codem::ScalarType dtype) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Thread& t) { return t.astype(dtype); });
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
  if (!defined()) return "{ \"_type\": \"Stream\", \"defined\": false }";

  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Stream\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"kind\": \"" << ptype() << "\",\n";
  oss << "  \"dtype\": \"" << json_escape(dtype()) << "\",\n";
  oss << "  \"size\": " << size() << ",\n";
  oss << "  \"values\": [";

  switch (kind()) {
    case StreamKind::Series: {
      const auto& s = as_series();
      if (s.scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto values = s.to_vec<bool>();
        for (std::size_t i = 0; i < values.size(); ++i) {
          if (i == 0) oss << "\n";
          oss << "    " << (values[i] ? "true" : "false");
          if (i + 1 < values.size()) oss << ",";
          oss << "\n";
        }
      } else if (s.scalar_type() == pluss::codem::ScalarType::Int64) {
        const auto values = s.to_vec<std::int64_t>();
        for (std::size_t i = 0; i < values.size(); ++i) {
          if (i == 0) oss << "\n";
          oss << "    " << values[i];
          if (i + 1 < values.size()) oss << ",";
          oss << "\n";
        }
      } else {
        const auto values = s.to(pluss::codem::ScalarType::Float64).to_vec<double>();
        for (std::size_t i = 0; i < values.size(); ++i) {
          if (i == 0) oss << "\n";
          oss << "    " << values[i];
          if (i + 1 < values.size()) oss << ",";
          oss << "\n";
        }
      }
      break;
    }
    case StreamKind::Note: {
      const auto& note = as_note();
      for (std::int64_t i = 0; i < note.numel(); ++i) {
        if (i == 0) oss << "\n";
        oss << "    \"" << json_escape(note.at(i)) << "\"";
        if (i + 1 < note.numel()) oss << ",";
        oss << "\n";
      }
      break;
    }
    case StreamKind::Timeline: {
      const auto values = as_timeline().to_epoch_nanos_vector();
      for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == 0) oss << "\n";
        oss << "    " << values[i];
        if (i + 1 < values.size()) oss << ",";
        oss << "\n";
      }
      break;
    }
    case StreamKind::TimeIndex: {
      const auto values = as_timeindex().timeline().to_epoch_nanos_vector();
      for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == 0) oss << "\n";
        oss << "    " << values[i];
        if (i + 1 < values.size()) oss << ",";
        oss << "\n";
      }
      break;
    }
  }

  if (size() > 0) oss << "  ";
  oss << "]";

  if (detail::has_validity_mask(*this)) {
    const auto valid = detail::validity_mask(*this)->to_vec<bool>();
    oss << ",\n";
    oss << "  \"validity\": [";
    for (std::size_t i = 0; i < valid.size(); ++i) {
      if (i == 0) oss << "\n";
      oss << "    " << (valid[i] ? "true" : "false");
      if (i + 1 < valid.size()) oss << ",";
      oss << "\n";
    }
    if (!valid.empty()) oss << "  ";
    oss << "]";
  }

  if (has_label()) {
    const auto label_values = label()->cast().note().as_note();
    oss << ",\n";
    oss << "  \"labels\": [";
    for (std::int64_t i = 0; i < label_values.numel(); ++i) {
      if (i == 0) oss << "\n";
      oss << "    \"" << json_escape(label_values.at(i)) << "\"";
      if (i + 1 < label_values.numel()) oss << ",";
      oss << "\n";
    }
    if (label_values.numel() > 0) oss << "  ";
    oss << "]";
  }

  oss << "\n}";
  return oss.str();
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
