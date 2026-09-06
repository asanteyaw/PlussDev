<!-- pluss-hand-authored -->

# Series::storage_version

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::storage_version() const -> std::int64_t
```

## What It Does

`storage_version()` returns the current mutation version counter for the
storage behind this `Series`.

This is a low-level inspection method, but it has a practical use: it tells you
whether in-place storage-changing work has happened since an earlier point in
time.

That makes it valuable in debugging and in understanding why reverse-mode
safety checks care about post-capture mutation.

## Parameters

None.

## Returns

`std::int64_t`
    The current storage version counter.

## How To Read It

The important interpretation rule is simple:

- if the value stays the same, no version-bumping mutation happened on that
  storage between the two reads
- if the value increases, some in-place storage mutation happened

The exact starting number is not the important contract. The meaningful signal
is change versus no change.

## Examples

Example 1. Non-mutating value reads leave the version unchanged.

```cpp
auto x = pluss::make_series(1.0).to(pluss::ScalarType::Float64);
std::cout << "before=" << x.storage_version() << "\n";
(void)x.exp();
std::cout << "after=" << x.storage_version() << "\n";
```

### Output

```text
before=0
after=0
```

The out-of-place `exp()` computation produced a new result, but it did not
mutate `x`'s own storage.

Example 2. An in-place operation bumps the version counter.

```cpp
auto x = pluss::make_series(1.0).to(pluss::ScalarType::Float64);
std::cout << "before=" << x.storage_version() << "\n";
x.relu_();
std::cout << "after=" << x.storage_version() << "\n";
```

### Output

```text
before=0
after=2
```

The key fact is the increase, not the exact increment size on this particular
operation.

## Gotchas And Failure Modes

- `storage_version()` is an inspection counter, not a data checksum.
- The meaningful contract is monotonic change detection, not a specific initial
  value or a guaranteed increment size of exactly `1`.
- Out-of-place methods can compute new values without changing this counter on
  the original series.

## Summary

Use `storage_version()` when you need a direct signal for "did this storage get
mutated in place since I last checked?"

## See Also

- [`Series::detach_`](detach_.md): one of several state-changing methods users
  may compare against storage mutation questions.
- [`Series::reverse`](reverse.md): reverse-mode safety is sensitive to
  post-capture mutation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stat.html">
    <span class="label">Previous</span>
    <span class="title">stat</span>
  </a>
  <a class="next-link" href="stride.html">
    <span class="label">Next</span>
    <span class="title">stride</span>
  </a>
</div>
