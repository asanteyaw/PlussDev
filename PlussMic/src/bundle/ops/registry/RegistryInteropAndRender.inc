pluss::kindle::Catalog Registry::kindelize() const {
  ensure_stream_compatible("Registry::kindelize");
  std::vector<std::pair<std::string, pluss::kindle::Stream>> out;
  out.reserve(static_cast<std::size_t>(nslots()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().slots_[i].kindelize());
  }
  return pluss::kindle::Catalog(std::move(out));
}

pluss::table::DataFrame Registry::tabelize() const {
  return tabelize(pluss::kindle::TabelizePolicy{});
}

pluss::table::DataFrame Registry::tabelize(const pluss::kindle::TabelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Registry::tabelize: registry undefined");
  return kindelize().tabelize(policy);
}

namespace {

pluss::table::DataFrame registry_to_io_frame(const Registry& registry) {
  auto catalog = registry.kindelize();
  pluss::kindle::TabelizePolicy policy;
  policy.align = pluss::kindle::Align::Outer;
  policy.align_by = pluss::kindle::AlignBy::Pos;
  policy.fill = pluss::kindle::FillPolicy::Null;
  policy.dtype = pluss::kindle::DTypePolicy::BestEffort;
  policy.collision = pluss::kindle::CollisionPolicy::Suffix;
  policy.include_label_feature = false;
  return catalog.tabelize(policy);
}

} // namespace

void Registry::save_csv(std::string_view filepath) const {
  save_csv(filepath, {});
}

void Registry::save_csv(std::string_view filepath, const pluss::loader::Config& config) const {
  registry_to_io_frame(*this).save_csv(filepath, config);
}

void Registry::save_txt(std::string_view filepath) const {
  save_txt(filepath, {});
}

void Registry::save_txt(std::string_view filepath, const pluss::loader::Config& config) const {
  registry_to_io_frame(*this).save_txt(filepath, config);
}

void Registry::save_excel(std::string_view filepath) const {
  save_excel(filepath, {});
}

void Registry::save_excel(std::string_view filepath, const pluss::loader::Config& config) const {
  registry_to_io_frame(*this).save_excel(filepath, config);
}

void Registry::save_json(std::string_view filepath) const {
  save_json(filepath, {});
}

void Registry::save_json(std::string_view filepath, const pluss::loader::Config& config) const {
  if (is_stream_compatible()) {
    registry_to_io_frame(*this).save_json(filepath, config);
    return;
  }
  (void)config;
  std::ofstream out(std::string(filepath), std::ios::out | std::ios::trunc | std::ios::binary);
  PLUSS_CHECK(out.is_open(), "Registry::save_json: failed to open file '" + std::string(filepath) + "'");
  out << to_json();
}

void Registry::save_parquet(std::string_view filepath) const {
  save_parquet(filepath, {});
}

void Registry::save_parquet(std::string_view filepath, const pluss::loader::Config& config) const {
  registry_to_io_frame(*this).save_parquet(filepath, config);
}

void Registry::save_as(std::string_view target_type,
                       std::string_view filepath,
                       const pluss::loader::Config& config) const {
  const auto target = lower_copy(trim_copy(target_type));
  if (target == "json") {
    save_json(filepath, config);
    return;
  }
  registry_to_io_frame(*this).save_as(target_type, filepath, config);
}

std::string Registry::to_string() const {
  if (!defined()) return "{ \"_type\": \"Registry\", \"defined\": false }";

  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Registry\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"nfields\": " << nfields() << ",\n";
  oss << "  \"fields\": {\n";
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& slot = impl_ref_().slots_[i];
    auto type_counts = slot.type_counts();
    std::vector<std::pair<std::string, std::int64_t>> type_items(type_counts.begin(), type_counts.end());
    std::sort(type_items.begin(), type_items.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    oss << "    \"" << json_escape(key) << "\": {\n";
    oss << "      \"size\": " << slot.size() << ",\n";
    oss << "      \"types\": {";
    if (!type_items.empty()) oss << "\n";
    for (std::size_t j = 0; j < type_items.size(); ++j) {
      oss << "        \"" << json_escape(type_items[j].first) << "\": " << type_items[j].second;
      if (j + 1 < type_items.size()) oss << ",";
      oss << "\n";
    }
    if (!type_items.empty()) oss << "      ";
    oss << "}\n";
    oss << "    }";
    if (i + 1 < impl_ref_().keys_.size()) oss << ",";
    oss << "\n";
  }
  oss << "  }\n";
  oss << "}";
  return oss.str();
}

std::string Registry::to_json() const {
  if (!defined()) return "{ \"_type\": \"Registry\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Registry\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"nfields\": " << nfields() << ",\n";
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
