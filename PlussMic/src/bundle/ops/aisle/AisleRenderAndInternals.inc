std::string Aisle::to_string() const { return to_json(); }

std::string Aisle::to_json() const {
  if (!defined()) return "{ \"_type\": \"Aisle\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Aisle\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"depth\": " << depth() << ",\n";
  oss << "  \"labels\": " << (labels().has_value() ? "true" : "false") << ",\n";
  oss << "  \"registries\": [\n";
  for (std::int64_t i = 0; i < depth(); ++i) {
    oss << registry(i).to_json();
    if (i + 1 < depth()) oss << ",";
    oss << "\n";
  }
  oss << "  ]\n";
  oss << "}";
  return oss.str();
}

void Aisle::show(std::ostream& os) const { os << to_string(); }

const detail::AisleImpl& Aisle::impl_ref_() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Aisle& aisle) {
  aisle.show(os);
  return os;
}

