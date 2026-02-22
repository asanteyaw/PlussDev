const detail::ArchiveImpl& Archive::impl_ref_() const {
  PLUSS_CHECK(defined(), "Archive: undefined handle");
  return *impl_;
}

