std::optional<TableMeta> Nexus::meta(std::string_view key) const {
  return table_meta(key);
}

Nexus Nexus::meta(std::string_view key, const TableMeta& value) const {
  return set_table_meta(key, value);
}

Nexus& Nexus::meta_(std::string_view key, const TableMeta& value) {
  return set_table_meta_(key, value);
}

const std::vector<Relationship>& Nexus::links() const {
  return relationships();
}

Nexus Nexus::links(const Relationship& rel) const {
  return add_relationship(rel);
}

Nexus& Nexus::links_(const Relationship& rel) {
  return add_relationship_(rel);
}

Nexus Nexus::links(std::initializer_list<Relationship> rels) const {
  return add_relationships(rels);
}

Nexus& Nexus::links_(std::initializer_list<Relationship> rels) {
  return add_relationships_(rels);
}

DataFrame Nexus::duplicates(std::string_view table) const {
  return detect_duplicates(table);
}

DataFrame Nexus::duplicates(std::string_view table,
                            std::initializer_list<std::string_view> keys) const {
  return detect_duplicates(table, keys);
}

DataFrame Nexus::duplicates(std::string_view table,
                            pluss::msharp::RangeView::Of<std::string_view> keys) const {
  return detect_duplicates(table, keys);
}

bool Nexus::check(std::string_view name) const {
  return validate_relationship(name);
}

DataFrame Nexus::orphans(std::string_view relationship_name,
                         RelationshipSide side) const {
  return detect_orphans(relationship_name, side);
}

DataFrame Nexus::audit(std::string_view name) const {
  return relationship_audit(name);
}
