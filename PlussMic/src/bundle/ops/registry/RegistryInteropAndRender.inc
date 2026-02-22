    w.keys_.push_back(name);
    w.slots_.push_back(value);
  } else {
    w.slots_[static_cast<std::size_t>(it->second)] = value;
  }
  return *this;
}

pluss::kindle::Catalog Registry::kindelize() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  std::vector<std::pair<std::string, pluss::kindle::Stream>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i].kindelize());
  }
  return pluss::kindle::Catalog(std::move(out));
}

Registry Registry::from_kindle(const pluss::kindle::Catalog& catalog) {
  PLUSS_CHECK(catalog.defined(), "Registry::from_kindle: catalog undefined");
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(static_cast<std::size_t>(catalog.nstreams()));
  const auto streams = catalog.streams();
  const auto& names = catalog.keys();
  for (std::size_t i = 0; i < names.size(); ++i) {
    out.emplace_back(names[i], Slot::from_kindle(streams[i]));
  }
  return Registry(std::move(out));
}

std::string Registry::to_string() const { return to_json(); }

std::string Registry::to_json() const {
  if (!defined()) return "{ \"_type\": \"Registry\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Registry\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"nslots\": " << nslots() << ",\n";
  oss << "  \"fields\": {\n";
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    oss << "    \"" << json_escape(impl_ref_().keys_[i]) << "\": " << impl_ref_().slots_[i].to_json();
    if (i + 1 < impl_ref_().keys_.size()) oss << ",";
    oss << "\n";
  }
  oss << "  }\n";
  oss << "}";
  return oss.str();
}

void Registry::show(std::ostream& os) const { os << to_string(); }

Registry::const_iterator Registry::begin() const { return keys().begin(); }
Registry::const_iterator Registry::end() const { return keys().end(); }

const detail::RegistryImpl& Registry::impl_ref_() const {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  return *impl_;
}

detail::RegistryImpl& Registry::mut_impl_() {
  PLUSS_CHECK(defined(), "Registry: undefined handle");
  if (impl_.use_count() > 1) {
    std::vector<std::pair<std::string, Slot>> copied;
    copied.reserve(impl_ref_().keys_.size());
    for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
      copied.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i]);
    }
    impl_ = pluss::msharp::make_intrusive<detail::RegistryImpl>(std::move(copied));
  }
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Registry& registry) {
  registry.show(os);
  return os;
}

