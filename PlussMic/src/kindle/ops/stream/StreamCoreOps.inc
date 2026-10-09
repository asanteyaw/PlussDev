pluss::table::Slate Stream::to_slate() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = std::visit(
      [&](const auto& payload) {
        return pluss::table::Slate(payload, impl_ref_().validity_mask_);
      },
      impl_ref_().payload_);
  if (has_label()) {
    out = out.label().set(*label());
  }
  return out;
}

pluss::table::Slate Stream::to_slate(std::int64_t start, std::int64_t stop) const {
  const auto t = to_slate();
  return t.slice(start, stop);
}

pluss::table::Slate Stream::tabelize() const {
  return to_slate();
}

pluss::scribble::Slot Stream::scribbelize() const {
  PLUSS_CHECK(defined(), "Stream::scribbelize: stream undefined");
  const auto n = size();
  const auto has_validity = pluss::kindle::detail::has_validity_mask(*this);
  const auto valid = has_validity
                         ? pluss::kindle::detail::validity_mask(*this)->to_vec<bool>()
                         : std::vector<bool>{};
  std::vector<pluss::scribble::Slot::Value> out;
  out.reserve(static_cast<std::size_t>(n));

  switch (kind()) {
    case pluss::kindle::StreamKind::Series: {
      const auto s = as_series();
      if (s.scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto x = s.to_vec<bool>();
        for (std::int64_t i = 0; i < n; ++i) {
          if (has_validity && !valid[static_cast<std::size_t>(i)]) {
            out.emplace_back(std::monostate{});
          } else {
            auto scalar = pluss::Series::zeros({1}, pluss::codem::ScalarType::Bool);
            reinterpret_cast<bool*>(scalar.raw_data())[0] = x[static_cast<std::size_t>(i)];
            out.emplace_back(std::move(scalar));
          }
        }
      } else if (s.scalar_type() == pluss::codem::ScalarType::Int64) {
        const auto x = s.to_vec<std::int64_t>();
        for (std::int64_t i = 0; i < n; ++i) {
          if (has_validity && !valid[static_cast<std::size_t>(i)]) {
            out.emplace_back(std::monostate{});
          } else {
            out.emplace_back(pluss::make_series(x[static_cast<std::size_t>(i)], s.scalar_type()));
          }
        }
      } else {
        const auto x = s.to(pluss::codem::ScalarType::Float64).to_vec<double>();
        for (std::int64_t i = 0; i < n; ++i) {
          if (has_validity && !valid[static_cast<std::size_t>(i)]) {
            out.emplace_back(std::monostate{});
          } else {
            out.emplace_back(pluss::make_series(x[static_cast<std::size_t>(i)], s.scalar_type()));
          }
        }
      }
      break;
    }
    case pluss::kindle::StreamKind::Note: {
      const auto& note = as_note();
      for (std::int64_t i = 0; i < n; ++i) {
        if (has_validity && !valid[static_cast<std::size_t>(i)]) {
          out.emplace_back(std::monostate{});
        } else {
          out.emplace_back(pluss::make_note(note.at(i)));
        }
      }
      break;
    }
    case pluss::kindle::StreamKind::Timeline: {
      const auto& tl = as_timeline();
      const std::string tz_name(tl.tz());
      const auto x = tl.ticks();
      for (std::int64_t i = 0; i < n; ++i) {
        if (has_validity && !valid[static_cast<std::size_t>(i)]) {
          out.emplace_back(std::monostate{});
        } else {
          out.emplace_back(pluss::timeline::from_epoch(x[static_cast<std::size_t>(i)], tz_name));
        }
      }
      break;
    }
    case pluss::kindle::StreamKind::Duration: {
      const auto x = as_duration().host();
      for (std::int64_t i = 0; i < n; ++i) {
        if (has_validity && !valid[static_cast<std::size_t>(i)]) {
          out.emplace_back(std::monostate{});
        } else {
          out.emplace_back(pluss::duration::nanos(
              std::vector<std::int64_t>{x[static_cast<std::size_t>(i)]}));
        }
      }
      break;
    }
  }

  return pluss::scribble::Slot(std::move(out));
}

Stream Stream::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Slate& t) { return t.head(n); });
}

Stream Stream::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Slate& t) { return t.tail(n); });
}

Stream Stream::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Slate& t) { return t.slice(start, stop); });
}

Stream Stream::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Slate& t) { return t.take(indices); });
}

Stream Stream::filter(const pluss::Series& bool_mask) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Slate& t) { return t.filter(bool_mask); });
}

Stream Stream::dropnull() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto source = to_slate();
  const auto out = source.dropnull();
  if (!has_label()) return Stream::from_slate(out);
  const auto label_out = label()->filter(source.notnull().as_series());
  return Stream::from_slate(out, label_out);
}

Stream Stream::ffill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Slate& t) { return t.ffill(); });
}

Stream Stream::bfill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [](const pluss::table::Slate& t) { return t.bfill(); });
}

Stream Stream::distinct(DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto out = to_slate().distinct(options.dropnull);
  if (out.label().has()) {
    return Stream::from_slate(out, out.label().values());
  }
  return Stream::from_slate(out);
}

Stream Stream::freq(FreqOptions options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto out = pluss::table::detail::freq_count_slate(
      to_slate(), options.top, options.dropnull, "freq");
  if (out.label().has()) {
    return Stream::from_slate(out, out.label().values());
  }
  return Stream::from_slate(out);
}

Stream Stream::ndistinct(DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().ndistinct(options.dropnull).slate());
}

Stream Stream::map(const std::function<Stream(const Stream&)>& fn) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Stream::map: function is empty");
  const auto out = fn(*this);
  PLUSS_CHECK(out.defined(), "Stream::map: function returned undefined stream");
  PLUSS_CHECK(out.size() == size(),
              "Stream::map: output stream length must match input stream length");
  return out;
}

Stream Stream::select(pluss::msharp::RangeView::Of<pluss::table::Slate> conditions,
                      pluss::msharp::RangeView::Of<Stream> choices,
                      const Stream& default_value) {
  PLUSS_CHECK(!choices.empty(), "Stream::select: at least one choice is required");
  PLUSS_CHECK(default_value.defined(), "Stream::select: default value is undefined");

  std::vector<pluss::table::Slate> choice_slates;
  choice_slates.reserve(choices.size());
  bool any_labels = default_value.has_label();
  for (const auto& choice : choices) {
    PLUSS_CHECK(choice.defined(), "Stream::select: choice is undefined");
    any_labels = any_labels || choice.has_label();
    choice_slates.push_back(choice.to_slate());
  }

  const auto out_slate = pluss::table::Slate::select(
      conditions,
      pluss::msharp::RangeView::Of<pluss::table::Slate>(choice_slates),
      default_value.to_slate());

  if (!any_labels) {
    return Stream::from_slate(out_slate);
  }

  PLUSS_CHECK(default_value.has_label(),
              "Stream::select: label presence must match across choices and default");
  std::vector<pluss::table::Slate> choice_labels;
  choice_labels.reserve(choices.size());
  for (const auto& choice : choices) {
    PLUSS_CHECK(choice.has_label(),
                "Stream::select: label presence must match across choices and default");
    choice_labels.push_back(*choice.label());
  }

  const auto out_label = pluss::table::Slate::select(
      conditions,
      pluss::msharp::RangeView::Of<pluss::table::Slate>(choice_labels),
      *default_value.label());
  return Stream::from_slate(out_slate, out_label);
}

Stream Stream::astype(pluss::codem::ScalarType dtype) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_label(*this, [&](const pluss::table::Slate& t) { return t.astype(dtype); });
}

Stream Stream::sum() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().sum().slate());
}

Stream Stream::mean() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().mean().slate());
}

Stream Stream::min() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().min().slate());
}

Stream Stream::max() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().max().slate());
}

Stream Stream::var(bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().var(unbiased).slate());
}

Stream Stream::std(bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_slate(to_slate().std(unbiased).slate());
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

const pluss::Duration& Stream::as_duration() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Duration>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Duration");
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
      const auto values = as_timeline().ticks();
      for (std::size_t i = 0; i < values.size(); ++i) {
        if (i == 0) oss << "\n";
        oss << "    " << values[i];
        if (i + 1 < values.size()) oss << ",";
        oss << "\n";
      }
      break;
    }
    case StreamKind::Duration: {
      const auto values = as_duration().host();
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

std::uint64_t Stream::fingerprint() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto mix = [](std::uint64_t h, std::uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
  };
  std::uint64_t h = mix(0, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(impl_.get())));
  h = mix(h, static_cast<std::uint64_t>(size()));
  switch (kind()) {
    case StreamKind::Series:
      h = mix(h, static_cast<std::uint64_t>(as_series().storage_version()));
      break;
    case StreamKind::Timeline:
      h = mix(h, static_cast<std::uint64_t>(as_timeline().epoch_nanos().storage_version()));
      break;
    default:
      break;  // Note and Duration payloads are never edited in place
  }
  if (detail::has_validity_mask(*this)) {
    h = mix(h, static_cast<std::uint64_t>(detail::validity_mask(*this)->storage_version()));
  }
  return h;
}
