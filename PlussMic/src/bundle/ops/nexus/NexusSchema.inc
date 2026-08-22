bool Nexus::has_table_meta(std::string_view key) const {
  const auto& src = impl_ref_();
  return src.table_meta_.find(std::string(key)) != src.table_meta_.end();
}

std::optional<TableMeta> Nexus::table_meta(std::string_view key) const {
  const auto& src = impl_ref_();
  const auto it = src.table_meta_.find(std::string(key));
  if (it == src.table_meta_.end()) {
    return std::nullopt;
  }
  return it->second;
}

Nexus Nexus::set_table_meta(std::string_view key, const TableMeta& meta) const {
  auto out = *this;
  out.set_table_meta_(key, meta);
  return out;
}

Nexus& Nexus::set_table_meta_(std::string_view key, const TableMeta& meta) {
  auto& dst = mut_impl_();
  require_known_table(dst, key, "Nexus::set_table_meta");
  dst.table_meta_[std::string(key)] = meta;
  invalidate_nexus_runtime(dst);
  return *this;
}

std::int64_t Nexus::nrelationships() const {
  return static_cast<std::int64_t>(impl_ref_().relationships_.size());
}

const std::vector<Relationship>& Nexus::relationships() const {
  return impl_ref_().relationships_;
}

bool Nexus::has_relationship(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.rel_name_to_pos_.find(std::string(name)) != src.rel_name_to_pos_.end();
}

std::optional<Relationship> Nexus::relationship(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.rel_name_to_pos_.find(std::string(name));
  if (it == src.rel_name_to_pos_.end()) {
    return std::nullopt;
  }
  return src.relationships_[static_cast<std::size_t>(it->second)];
}

Nexus Nexus::add_relationship(const Relationship& rel) const {
  auto out = *this;
  out.add_relationship_(rel);
  return out;
}

Nexus& Nexus::add_relationship_(const Relationship& rel) {
  auto& dst = mut_impl_();
  validate_relationship_payload(dst, rel, "Nexus::add_relationship");
  PLUSS_CHECK(dst.rel_name_to_pos_.find(rel.name) == dst.rel_name_to_pos_.end(),
              "Nexus::add_relationship: duplicate relationship '" + rel.name + "'");
  dst.rel_name_to_pos_.emplace(rel.name, static_cast<std::int64_t>(dst.relationships_.size()));
  dst.relationships_.push_back(rel);
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::add_relationships(std::initializer_list<Relationship> rels) const {
  auto out = *this;
  out.add_relationships_(rels);
  return out;
}

Nexus& Nexus::add_relationships_(std::initializer_list<Relationship> rels) {
  auto& dst = mut_impl_();
  dst.relationships_.reserve(dst.relationships_.size() + rels.size());
  dst.rel_name_to_pos_.reserve((dst.relationships_.size() + rels.size()) * 2 + 1);
  for (const auto& rel : rels) {
    validate_relationship_payload(dst, rel, "Nexus::add_relationships");
    PLUSS_CHECK(dst.rel_name_to_pos_.find(rel.name) == dst.rel_name_to_pos_.end(),
                "Nexus::add_relationships: duplicate relationship '" + rel.name + "'");
    dst.rel_name_to_pos_.emplace(rel.name, static_cast<std::int64_t>(dst.relationships_.size()));
    dst.relationships_.push_back(rel);
  }
  invalidate_nexus_runtime(dst);
  return *this;
}
