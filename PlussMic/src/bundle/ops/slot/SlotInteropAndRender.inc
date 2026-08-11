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
