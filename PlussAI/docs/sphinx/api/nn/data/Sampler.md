# `pluss::loader::Sampler`

The index-sampling strategy interface
[`DataLoaderOptions::sampler`](DataLoaderOptions.md) takes -- an
alternative to the plain `shuffle` bool for anything more specific than
"shuffled or not": a fixed subset, weighted sampling, or a custom
strategy entirely. One method: `sample(size, epoch_seed)` returns the
*full* epoch order up front, as a plain vector -- not a lazy per-element
iterator, matching how [`DataLoader`](DataLoader.md) already builds its
own internal order vector.

## Signature

```cpp
virtual pluss::loader::Sampler::~Sampler() = default;
virtual std::vector<std::int64_t> pluss::loader::Sampler::sample(std::int64_t size, std::uint64_t epoch_seed) const = 0;
```

`Sampler` has no public constructor of its own -- it is an abstract
interface; construct one of its four concrete subclasses
([`SequentialSampler`](SequentialSampler.md),
[`RandomSampler`](RandomSampler.md),
[`SubsetRandomSampler`](SubsetRandomSampler.md),
[`WeightedRandomSampler`](WeightedRandomSampler.md)) instead, or
implement a custom one.

## Construction

Not applicable to `Sampler` itself. Construct a concrete subclass, wrap
it in a `shared_ptr`, and assign it to
[`DataLoaderOptions::sampler`](DataLoaderOptions.md):

```cpp
pluss::loader::DataLoaderOptions options{};
options.sampler = std::make_shared<pluss::loader::SubsetRandomSampler>(std::vector<std::int64_t>{0, 2, 4});
```

`sampler` and `shuffle` are mutually exclusive -- `DataLoader` throws if
both are set.

## State

None on `Sampler` itself.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `sample(size, epoch_seed)` | `vector<int64_t>` | [`sample`](Sampler/sample.md) |

## Changelog

- **0.1** — the one-method index-sampling interface, with an eager
  (not lazy) per-epoch order.

## See Also

- [`DataLoaderOptions`](DataLoaderOptions.md) -- `sampler` is where this
  gets wired in.
- [`SequentialSampler`](SequentialSampler.md) /
  [`RandomSampler`](RandomSampler.md) /
  [`SubsetRandomSampler`](SubsetRandomSampler.md) /
  [`WeightedRandomSampler`](WeightedRandomSampler.md) -- the four
  concrete strategies.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Sampler/Introduction
Sampler/sample
```
<!-- apiref:toctree:end -->
