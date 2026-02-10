#define TORCH_ASSERT_NO_OPERATORS
#include <PlussQit/core/NamedSeries.h>
#include <PlussQit/core/SeriesBase.h>

namespace pluss {

thread_local static bool NamesMode_enabled = true;

bool NamesMode::is_enabled() {
  return NamesMode_enabled;
}

void NamesMode::set_enabled(bool enabled) {
  NamesMode_enabled = enabled;
  MSharp::impl::tls_set_dispatch_key_excluded(DispatchKey::Named, !enabled);
}

const SeriesBase& internal_set_names_inplace(const SeriesBase& series, std::optional<DimnameList> names) {
  impl::internal_set_names_inplace(series.unsafeGetSeriesImpl(), names, /*validate_names=*/true);
  return series;
}

const SeriesBase& internal_set_names_inplace(const SeriesBase& series, std::vector<Dimname>&& names, bool validate_names) {
  impl::internal_set_names_inplace(series.unsafeGetSeriesImpl(), std::move(names), validate_names);
  return series;
}

DimnameList default_names(size_t len) {
  static std::vector<Dimname> all_unnamed(kMaxNamedSeriesDim, Dimname::wildcard());
    TORCH_INTERNAL_ASSERT(
        len <= kMaxNamedSeriesDim,
        "Only series with up to ", kMaxNamedSeriesDim, " are supported.");
  return DimnameList(&all_unnamed.front(), len);
}

static void check_unique_names(DimnameList names) {
  // Strategy: Compare each element with the ones that come after it.
  // Although this is O(N^2), in practice N is small (no more than 25).
  for (auto it = names.begin(); it != names.end(); ++it) {
    if (it->isWildcard()) continue;
    auto dup = std::find(it + 1, names.end(), *it);
    while (dup != names.end()) {
      TORCH_CHECK(false,
          "Cannot construct a series with duplicate names. Got names: ",
          names, ".");
    }
  }
}

void check_names_valid_for(const SeriesBase& series, DimnameList names) {
  return impl::check_names_valid_for(series.unsafeGetSeriesImpl(), names);
}

void check_names_valid_for(size_t series_dim, DimnameList names) {
  TORCH_CHECK(
    series_dim <= kMaxNamedSeriesDim,
    "Named series only support up to ", kMaxNamedSeriesDim, " dims: "
    "Attempted to create a series with dim ", series_dim, " with names ", names);
  TORCH_CHECK(series_dim == names.size(),
    "Number of names (", names.size(), ") and "
    "number of dimensions in series (", series_dim, ") ",
    "do not match. Attempted to create a series with names ", names);
  check_unique_names(names);
}

namespace impl {

static NamedSeriesMeta* get_named_series_meta(SeriesImpl* impl) {
  if (!NamesMode::is_enabled()) {
    return nullptr;
  }
  return static_cast<NamedSeriesMeta*>(impl->named_series_meta());
}

static const NamedSeriesMeta* get_named_series_meta(const SeriesImpl* impl) {
  if (!NamesMode::is_enabled()) {
    return nullptr;
  }
  return static_cast<const NamedSeriesMeta*>(impl->named_series_meta());
}

void check_names_valid_for(SeriesImpl* impl, DimnameList names) {
  check_names_valid_for(impl->dim(), names);
}

void internal_set_names_inplace(SeriesImpl* impl, std::optional<DimnameList> names, bool validate_names) {
  TORCH_CHECK(impl->layout() == Layout::Strided,
      "NYI: named series only support strided layout");
  TORCH_CHECK(impl->device().is_cpu() || impl->device().is_cuda() || impl->device().is_xpu() || impl->device().is_privateuseone(),
      "NYI: named series only support CPU, CUDA, XPU or ", MSharp::get_privateuse1_backend(), " series.");
  if (!names) {
    impl->set_named_series_meta(nullptr);
    return;
  }
  if (validate_names) {
    check_names_valid_for(impl, *names);
  }
  // Do this after validation!
  if (std::all_of(names->begin(), names->end(), [](const Dimname& n) { return n.isWildcard(); })) {
    impl->set_named_series_meta(nullptr);
    return;
  }
  auto* meta = get_named_series_meta(impl);
  if (meta == nullptr) {
    // Constructor is private
    impl->set_named_series_meta(std::make_unique<NamedSeriesMeta>(NamedSeriesMeta::HasNonWildcard, *names));
  } else {
    meta->set_names(NamedSeriesMeta::HasNonWildcard, *names);
  }
}

void internal_set_names_inplace(SeriesImpl* impl, std::vector<Dimname>&& names, bool validate_names) {
  if (validate_names) {
    check_names_valid_for(impl, names);
  }
  // Do this after validation!
  if (std::all_of(names.begin(), names.end(), [](const Dimname& n) { return n.isWildcard(); })) {
    impl->set_named_series_meta(nullptr);
    return;
  }
  auto* meta = get_named_series_meta(impl);
  if (meta == nullptr) {
    impl->set_named_series_meta(std::make_unique<NamedSeriesMeta>(NamedSeriesMeta::HasNonWildcard, std::move(names)));
  } else {
    meta->set_names(NamedSeriesMeta::HasNonWildcard, std::move(names));
  }
}

std::optional<DimnameList> get_opt_names(const SeriesImpl* impl) {
  const auto* meta = get_named_series_meta(impl);
  if (meta == nullptr) {
    return std::nullopt;
  } else {
    return meta->names();
  }
}

DimnameList get_names(const SeriesImpl* impl) {
  auto maybe_names = get_opt_names(impl);
  if (maybe_names) {
    return *maybe_names;
  }
  return default_names(impl->dim());
}

bool has_names(const SeriesImpl* impl) {
  return impl->has_named_series_meta() && NamesMode::is_enabled();
}

} // namespace impl

} // namespace pluss
