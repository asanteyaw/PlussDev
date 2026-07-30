Slot Slot::from_kindle(const pluss::kindle::Stream& stream) {
  PLUSS_CHECK(stream.defined(), "Slot::from_kindle: stream undefined");
  const auto n = stream.size();
  const auto has_validity = pluss::kindle::detail::has_validity_mask(stream);
  const auto valid = has_validity
                         ? pluss::kindle::detail::validity_mask(stream)->to_vec<bool>()
                         : std::vector<bool>{};
  std::vector<Value> out;
  out.reserve(static_cast<std::size_t>(n));
  switch (stream.kind()) {
    case pluss::kindle::StreamKind::Series: {
      const auto s = stream.as_series();
      if (s.scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto x = s.to_vec<bool>();
        for (std::int64_t i = 0; i < n; ++i) {
          if (has_validity && !valid[static_cast<std::size_t>(i)]) {
            out.emplace_back(std::monostate{});
          } else {
            out.emplace_back(bool_slot_value(x[static_cast<std::size_t>(i)]));
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
      const auto& nte = stream.as_note();
      for (std::int64_t i = 0; i < n; ++i) {
        if (has_validity && !valid[static_cast<std::size_t>(i)]) {
          out.emplace_back(std::monostate{});
        } else {
          out.emplace_back(pluss::make_note(nte.at(i)));
        }
      }
      break;
    }
    case pluss::kindle::StreamKind::Timeline: {
      const auto x = stream.as_timeline().to_epoch_nanos_vector();
      for (std::int64_t i = 0; i < n; ++i) {
        if (has_validity && !valid[static_cast<std::size_t>(i)]) {
          out.emplace_back(std::monostate{});
        } else {
          out.emplace_back(pluss::from_epoch_nanos(x[static_cast<std::size_t>(i)]));
        }
      }
      break;
    }
    case pluss::kindle::StreamKind::TimeIndex: {
      const auto x = stream.as_timeindex().timeline().to_epoch_nanos_vector();
      for (std::int64_t i = 0; i < n; ++i) {
        if (has_validity && !valid[static_cast<std::size_t>(i)]) {
          out.emplace_back(std::monostate{});
        } else {
          out.emplace_back(pluss::make_timeindex(
              std::vector<std::int64_t>{x[static_cast<std::size_t>(i)]},
              false));
        }
      }
      break;
    }
  }
  auto impl = pluss::msharp::make_intrusive<detail::SlotImpl>(std::move(out));
  impl->profile_cache_ = SlotProfile{
      .kind = n == 0 ? SlotKind::Empty : slot_kind_from_stream_kind(stream.kind()),
      .has_null = has_validity,
      .stream_compatible = !has_validity};
  return Slot(std::move(impl));
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
