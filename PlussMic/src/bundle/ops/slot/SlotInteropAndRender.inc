Slot Slot::from_kindle(const pluss::kindle::Stream& stream) {
  PLUSS_CHECK(stream.defined(), "Slot::from_kindle: stream undefined");
  const auto n = stream.size();
  std::vector<Value> out;
  out.reserve(static_cast<std::size_t>(n));
  switch (stream.kind()) {
    case pluss::kindle::StreamKind::Series: {
      const auto s = stream.as_series();
      if (s.scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto x = s.to_vec<bool>();
        for (bool v : x) out.push_back(v);
      } else if (s.scalar_type() == pluss::codem::ScalarType::Int64) {
        const auto x = s.to_vec<std::int64_t>();
        for (auto v : x) out.push_back(v);
      } else {
        const auto x = s.to(pluss::codem::ScalarType::Float64).to_vec<double>();
        for (auto v : x) out.push_back(v);
      }
      break;
    }
    case pluss::kindle::StreamKind::Note: {
      const auto& nte = stream.as_note();
      for (std::int64_t i = 0; i < n; ++i) out.push_back(std::string(nte.at(i)));
      break;
    }
    case pluss::kindle::StreamKind::Timeline: {
      const auto x = stream.as_timeline().to_epoch_nanos_vector();
      for (auto v : x) out.push_back(v);
      break;
    }
    case pluss::kindle::StreamKind::TimeIndex: {
      const auto x = stream.as_timeindex().timeline().to_epoch_nanos_vector();
      for (auto v : x) out.push_back(v);
      break;
    }
  }
  return Slot(std::move(out));
}

std::string Slot::to_string() const {
  return to_json();
}

std::string Slot::to_json() const {
  if (!defined()) return "{ \"_type\": \"Slot\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Slot\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"size\": " << size() << ",\n";
  oss << "  \"values\": [";
  if (!values().empty()) oss << "\n";
  for (std::size_t i = 0; i < values().size(); ++i) {
    oss << "    " << value_to_json(values()[i]);
    if (i + 1 < values().size()) oss << ",";
    oss << "\n";
  }
  if (!values().empty()) oss << "  ";
  oss << "]\n";
  oss << "}";
  return oss.str();
}

void Slot::show(std::ostream& os) const { os << to_string(); }

const detail::SlotImpl& Slot::impl_ref_() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return *impl_;
}

detail::SlotImpl& Slot::mut_impl_() {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (impl_.use_count() > 1) {
    impl_ = pluss::msharp::make_intrusive<detail::SlotImpl>(impl_ref_().values_);
  }
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Slot& slot) {
  slot.show(os);
  return os;
}

