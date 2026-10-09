namespace {

std::optional<std::string> maybe_pull_alias(std::string_view alias) {
  if (alias.empty()) {
    return std::nullopt;
  }
  return std::string(alias);
}

PullSpec make_pull_spec(std::string_view source,
                        PullOp op,
                        std::string_view alias,
                        bool prefer_feature_alias) {
  PLUSS_CHECK(!source.empty(), "pull spec source cannot be empty");
  return PullSpec{
      .source = std::string(source),
      .op = op,
      .alias = maybe_pull_alias(alias),
      .window = std::nullopt,
      .prefer_feature_alias = prefer_feature_alias,
  };
}

std::string qualify_pull_source(std::string_view table, std::string_view feature) {
  PLUSS_CHECK(!table.empty(), "pull table scope cannot be empty");
  PLUSS_CHECK(!feature.empty(), "pull feature cannot be empty");
  return fmt::format("{}.{}", table, feature);
}

} // namespace

namespace nexus {

PullSpec take(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Take, alias, true);
}

PullSpec count(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Count, alias, false);
}

PullSpec ndistinct(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Ndistinct, alias, false);
}

PullSpec sum(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Sum, alias, false);
}

PullSpec mean(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Mean, alias, false);
}

PullSpec median(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Median, alias, false);
}

PullSpec min(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Min, alias, false);
}

PullSpec max(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Max, alias, false);
}

PullSpec std(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Std, alias, false);
}

PullSpec var(std::string_view source, std::string_view alias) {
  return make_pull_spec(source, PullOp::Var, alias, false);
}

} // namespace nexus

NexusPull::NexusPull(Nexus nexus, std::string root_table)
    : nexus_(std::move(nexus)), root_table_(std::move(root_table)) {}

NexusPull& NexusPull::add(const PullSpec& spec) {
  specs_.push_back(spec);
  return *this;
}

NexusPull& NexusPull::add(std::initializer_list<PullSpec> specs) {
  specs_.insert(specs_.end(), specs.begin(), specs.end());
  return *this;
}

NexusPull& NexusPull::take(pluss::msharp::RangeView::Of<std::string_view> sources) {
  for (const auto source : sources) {
    add(nexus::take(source));
  }
  return *this;
}

NexusPull& NexusPull::take(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  for (const auto& [source, alias] : fields) {
    add(nexus::take(source, alias));
  }
  return *this;
}

NexusPull& NexusPull::take(std::string_view table,
                           pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  for (const auto& [source, alias] : fields) {
    add(nexus::take(qualify_pull_source(table, source), alias));
  }
  return *this;
}

NexusPull& NexusPull::take(std::string_view source, std::string_view alias) {
  return add(nexus::take(source, alias));
}

NexusPull& NexusPull::count(std::string_view source, std::string_view alias) {
  return add(nexus::count(source, alias));
}

NexusPull& NexusPull::ndistinct(std::string_view source, std::string_view alias) {
  return add(nexus::ndistinct(source, alias));
}

NexusPull& NexusPull::sum(std::string_view source, std::string_view alias) {
  return add(nexus::sum(source, alias));
}

NexusPull& NexusPull::mean(std::string_view source, std::string_view alias) {
  return add(nexus::mean(source, alias));
}

NexusPull& NexusPull::median(std::string_view source, std::string_view alias) {
  return add(nexus::median(source, alias));
}

NexusPull& NexusPull::min(std::string_view source, std::string_view alias) {
  return add(nexus::min(source, alias));
}

NexusPull& NexusPull::max(std::string_view source, std::string_view alias) {
  return add(nexus::max(source, alias));
}

NexusPull& NexusPull::std(std::string_view source, std::string_view alias) {
  return add(nexus::std(source, alias));
}

NexusPull& NexusPull::var(std::string_view source, std::string_view alias) {
  return add(nexus::var(source, alias));
}

NexusPullAgg NexusPull::agg(std::string_view table) {
  PLUSS_CHECK(!table.empty(), "NexusPull::agg: table scope cannot be empty");
  return NexusPullAgg(*this, std::string(table));
}

const std::string& NexusPull::root() const noexcept {
  return root_table_;
}

const std::vector<PullSpec>& NexusPull::specs() const noexcept {
  return specs_;
}

pluss::table::DataFrame NexusPull::collect() const {
  return nexus_.pull(root_table_, specs_);
}

NexusPullAgg::NexusPullAgg(NexusPull& pull, std::string table_name)
    : pull_(&pull), table_name_(std::move(table_name)) {}

NexusPullAgg& NexusPullAgg::add_scoped(PullOp op, std::string_view feature, std::string_view alias) {
  PLUSS_CHECK(pull_ != nullptr, "NexusPullAgg is not attached to a parent pull builder");
  pull_->add(make_pull_spec(qualify_pull_source(table_name_, feature), op, alias, false));
  return *this;
}

NexusPullAgg& NexusPullAgg::add_scoped(
    PullOp op,
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  for (const auto& [feature, alias] : fields) {
    add_scoped(op, feature, alias);
  }
  return *this;
}

NexusPullAgg& NexusPullAgg::count(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Count, feature, alias);
}

NexusPullAgg& NexusPullAgg::count(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Count, fields);
}

NexusPullAgg& NexusPullAgg::ndistinct(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Ndistinct, feature, alias);
}

NexusPullAgg& NexusPullAgg::ndistinct(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Ndistinct, fields);
}

NexusPullAgg& NexusPullAgg::sum(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Sum, feature, alias);
}

NexusPullAgg& NexusPullAgg::sum(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Sum, fields);
}

NexusPullAgg& NexusPullAgg::mean(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Mean, feature, alias);
}

NexusPullAgg& NexusPullAgg::mean(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Mean, fields);
}

NexusPullAgg& NexusPullAgg::median(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Median, feature, alias);
}

NexusPullAgg& NexusPullAgg::median(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Median, fields);
}

NexusPullAgg& NexusPullAgg::min(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Min, feature, alias);
}

NexusPullAgg& NexusPullAgg::min(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Min, fields);
}

NexusPullAgg& NexusPullAgg::max(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Max, feature, alias);
}

NexusPullAgg& NexusPullAgg::max(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Max, fields);
}

NexusPullAgg& NexusPullAgg::std(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Std, feature, alias);
}

NexusPullAgg& NexusPullAgg::std(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Std, fields);
}

NexusPullAgg& NexusPullAgg::var(std::string_view feature, std::string_view alias) {
  return add_scoped(PullOp::Var, feature, alias);
}

NexusPullAgg& NexusPullAgg::var(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  return add_scoped(PullOp::Var, fields);
}

NexusPull& NexusPullAgg::end() const {
  PLUSS_CHECK(pull_ != nullptr, "NexusPullAgg is not attached to a parent pull builder");
  return *pull_;
}

std::optional<TableMeta> Nexus::meta(std::string_view key) const {
  return table_meta(key);
}

Nexus Nexus::meta(std::string_view key, const TableMeta& value) const {
  return set_table_meta(key, value);
}

Nexus Nexus::meta(std::initializer_list<std::pair<std::string_view, nexus::TableMeta>> values) const {
  auto out = *this;
  out.meta_(values);
  return out;
}

Nexus& Nexus::meta_(std::string_view key, const TableMeta& value) {
  return set_table_meta_(key, value);
}

Nexus& Nexus::meta_(std::initializer_list<std::pair<std::string_view, nexus::TableMeta>> values) {
  for (const auto& [key, value] : values) {
    meta_(key, value);
  }
  return *this;
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

pluss::table::DataFrame Nexus::duplicates(std::string_view table) const {
  return detect_duplicates(table);
}

pluss::table::DataFrame Nexus::duplicates(std::string_view table,
                            std::initializer_list<std::string_view> keys) const {
  return detect_duplicates(table, keys);
}

pluss::table::DataFrame Nexus::duplicates(std::string_view table,
                            pluss::msharp::RangeView::Of<std::string_view> keys) const {
  return detect_duplicates(table, keys);
}

bool Nexus::check(std::string_view name) const {
  return validate_relationship(name);
}

pluss::table::DataFrame Nexus::audit() const {
  return scan();
}

pluss::table::DataFrame Nexus::orphans(std::string_view relationship_name,
                         RelationshipSide side) const {
  return detect_orphans(relationship_name, side);
}

pluss::table::DataFrame Nexus::audit(std::string_view name) const {
  return relationship_audit(name);
}
