#pragma once

#include "pluss/config.h"

#include <optional>

namespace pluss {

struct SeriesStorageOptions final {
  std::optional<pluss::codem::ScalarType> dtype{};
  std::optional<pluss::codem::Platform> platform{};

  constexpr SeriesStorageOptions& with_dtype(pluss::codem::ScalarType value) noexcept {
    dtype = value;
    return *this;
  }

  constexpr SeriesStorageOptions& with_platform(pluss::codem::Platform value) noexcept {
    platform = value;
    return *this;
  }

  constexpr SeriesStorageOptions& with_config(const pluss::codem::Config& cfg) noexcept {
    dtype = cfg.dtype;
    platform = cfg.platform;
    return *this;
  }
};

struct SeriesAutodiffOptions final {
  std::optional<bool> tracks_slope{};
  std::optional<bool> retain_sensitivity{};

  constexpr SeriesAutodiffOptions& with_tracks_slope(bool value = true) noexcept {
    tracks_slope = value;
    return *this;
  }

  constexpr SeriesAutodiffOptions& with_retain_sensitivity(bool value = true) noexcept {
    retain_sensitivity = value;
    return *this;
  }
};

struct SeriesOptions final {
  SeriesStorageOptions storage{};
  SeriesAutodiffOptions autodiff{};

  constexpr SeriesOptions& with_dtype(pluss::codem::ScalarType value) noexcept {
    storage.with_dtype(value);
    return *this;
  }

  constexpr SeriesOptions& with_platform(pluss::codem::Platform value) noexcept {
    storage.with_platform(value);
    return *this;
  }

  constexpr SeriesOptions& with_config(const pluss::codem::Config& cfg) noexcept {
    storage.with_config(cfg);
    return *this;
  }

  constexpr SeriesOptions& with_tracks_slope(bool value = true) noexcept {
    autodiff.with_tracks_slope(value);
    return *this;
  }

  constexpr SeriesOptions& with_retain_sensitivity(bool value = true) noexcept {
    autodiff.with_retain_sensitivity(value);
    return *this;
  }

  constexpr const SeriesStorageOptions& storage_options() const noexcept { return storage; }
  constexpr const SeriesAutodiffOptions& autodiff_options() const noexcept { return autodiff; }
};

inline constexpr SeriesStorageOptions series_storage_options() noexcept {
  return SeriesStorageOptions{};
}

inline constexpr SeriesAutodiffOptions series_autodiff_options() noexcept {
  return SeriesAutodiffOptions{};
}

inline constexpr SeriesOptions series_options() noexcept {
  return SeriesOptions{};
}

} // namespace pluss
