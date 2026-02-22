pluss::kindle::Archive Atlas::kindelize() const {
  return kindelize(pluss::kindle::KindelizePolicy{});
}

pluss::kindle::Archive Atlas::kindelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Atlas::kindelize requires a defined Atlas");
  return pluss::kindle::Archive::kindelize(*this, policy);
}

Atlas Atlas::tabelize(const pluss::kindle::Archive& archive) {
  return tabelize(archive, pluss::kindle::TabelizePolicy{});
}

Atlas Atlas::tabelize(const pluss::kindle::Archive& archive,
                      const pluss::kindle::TabelizePolicy& policy) {
  PLUSS_CHECK(archive.defined(), "Atlas::tabelize requires a defined Archive");
  return archive.tabelize(policy);
}
