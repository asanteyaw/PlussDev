# API Reference

## Entry Point

All distribution functionality is available from:

```cpp
pluss::distr
```

That alias resolves to `pluss::subsp::stat::distr`, but user code should generally prefer the shorter public path.

## Core Functions

### `supports`

```cpp
bool pluss::distr::supports(std::string_view name) noexcept;
```

Returns whether a distribution name or alias is recognized.

### `sample`

```cpp
pluss::Series pluss::distr::sample(
    std::string_view name,
    std::span<const double> params,
    std::span<const std::int64_t> sizes,
    pluss::Config cfg = {},
    std::uint64_t seed = 0);
```

Convenience overloads also exist for:

- `initializer_list<double>` parameters
- `initializer_list<std::int64_t>` sizes
- zero-parameter distributions via `sample(name, sizes, cfg, seed)`

### `sample_like`

```cpp
pluss::Series pluss::distr::sample_like(
    std::string_view name,
    std::span<const double> params,
    const pluss::Series& like,
    std::uint64_t seed = 0);
```

Convenience overloads also exist for:

- `initializer_list<double>` parameters
- zero-parameter distributions via `sample_like(name, like, seed)`

`sample_like` preserves the shape, platform, and dtype of `like`.

### `pdf`

```cpp
pluss::Series pluss::distr::pdf(
    std::string_view name,
    const pluss::Series& x,
    std::span<const double> params = {});
```

### `logpdf`

```cpp
pluss::Series pluss::distr::logpdf(
    std::string_view name,
    const pluss::Series& x,
    std::span<const double> params = {});
```

### `cdf`

```cpp
pluss::Series pluss::distr::cdf(
    std::string_view name,
    const pluss::Series& x,
    std::span<const double> params = {});
```

### `mixture_normal`

```cpp
pluss::Series pluss::distr::mixture_normal(
    const pluss::Series& weights,
    const pluss::Series& means,
    const pluss::Series& stddevs,
    std::span<const std::int64_t> sizes,
    pluss::Config cfg = {},
    std::uint64_t seed = 0,
    bool use_sobol = false);
```

This samples from an `n`-component Gaussian mixture.

- `weights`, `means`, and `stddevs` must have the same length
- `weights` are normalized internally
- `use_sobol=true` uses Sobol-driven Gaussian draws

## Notes On Dtypes

- density-style functions return floating-point `Series`
- discrete samplers use natural integral output where appropriate
- `sample_like` is the shortest path when you want the result to match an existing tensor contract
