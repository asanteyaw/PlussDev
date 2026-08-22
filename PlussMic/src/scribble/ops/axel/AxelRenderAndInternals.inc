std::string Axel::to_string() const {
  if (!defined()) return "{ \"_type\": \"Axel\", \"defined\": false }";

  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Axel\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"depth\": " << depth() << ",\n";
  oss << "  \"labels\": " << (labels().has_value() ? "true" : "false") << ",\n";
  if (labels().has_value()) {
    auto counts = labels()->type_counts();
    std::vector<std::pair<std::string, std::int64_t>> type_items(counts.begin(), counts.end());
    std::sort(type_items.begin(), type_items.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    oss << "  \"label_summary\": {\n";
    oss << "    \"size\": " << labels()->size() << ",\n";
    oss << "    \"types\": {";
    if (!type_items.empty()) oss << "\n";
    for (std::size_t j = 0; j < type_items.size(); ++j) {
      oss << "      \"" << json_escape(type_items[j].first) << "\": " << type_items[j].second;
      if (j + 1 < type_items.size()) oss << ",";
      oss << "\n";
    }
    if (!type_items.empty()) oss << "    ";
    oss << "}\n";
    oss << "  },\n";
  }

  const std::int64_t d = depth();
  const bool needs_ellipsis = d > 6;
  const std::int64_t head_n = needs_ellipsis ? 3 : d;
  const std::int64_t tail_n = needs_ellipsis ? 2 : 0;

  oss << "  \"registries\": [\n";
  auto emit_registry_summary = [&](std::int64_t idx, bool trailing_comma) {
    const auto reg = registry(idx);
    const auto& field_vec = reg.fields();
    std::vector<std::string> fields(field_vec.begin(), field_vec.end());
    auto sizes = reg.sizes();
    std::vector<std::pair<std::string, std::int64_t>> size_items(sizes.begin(), sizes.end());
    std::sort(size_items.begin(), size_items.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    oss << "    {\n";
    oss << "      \"index\": " << idx << ",\n";
    oss << "      \"nfields\": " << reg.nfields() << ",\n";
    oss << "      \"fields\": [";
    if (!fields.empty()) oss << "\n";
    for (std::size_t j = 0; j < fields.size(); ++j) {
      oss << "        \"" << json_escape(fields[j]) << "\"";
      if (j + 1 < fields.size()) oss << ",";
      oss << "\n";
    }
    if (!fields.empty()) oss << "      ";
    oss << "],\n";
    oss << "      \"slot_sizes\": {";
    if (!size_items.empty()) oss << "\n";
    for (std::size_t j = 0; j < size_items.size(); ++j) {
      oss << "        \"" << json_escape(size_items[j].first) << "\": " << size_items[j].second;
      if (j + 1 < size_items.size()) oss << ",";
      oss << "\n";
    }
    if (!size_items.empty()) oss << "      ";
    oss << "}\n";
    oss << "    }";
    if (trailing_comma) oss << ",";
    oss << "\n";
  };

  for (std::int64_t i = 0; i < head_n; ++i) {
    const bool trailing = (i + 1 < head_n) || needs_ellipsis || (tail_n > 0);
    emit_registry_summary(i, trailing);
  }
  if (needs_ellipsis) {
    const auto skipped = d - head_n - tail_n;
    oss << "    { \"...\": \"" << skipped << " registries omitted\" },\n";
  }
  for (std::int64_t i = d - tail_n; i < d; ++i) {
    const bool trailing = i + 1 < d;
    emit_registry_summary(i, trailing);
  }

  oss << "  ]\n";
  oss << "}";
  return oss.str();
}

std::string Axel::to_json() const {
  if (!defined()) return "{ \"_type\": \"Axel\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Axel\",\n";
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

void Axel::show(std::ostream& os) const { os << to_string(); }

const detail::AxelImpl& Axel::impl_ref_() const {
  PLUSS_CHECK(defined(), "Axel: undefined handle");
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Axel& axel) {
  axel.show(os);
  return os;
}
