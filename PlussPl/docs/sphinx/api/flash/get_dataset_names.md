<!-- pluss-hand-authored -->

# flash::get_dataset_names

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
Note flash::get_dataset_names()
```

## What It Does

Returns the curated flash dataset registry as a `Note` of dataset names.

This function does not read a remote catalog and does not download anything. It is the
discovery surface for the persisted teaching datasets that
[`flash::load_dataset`](load_dataset.md) knows how to load.

At runtime, the name list comes from the installed dataset manifest when one is present.
If the manifest is not available yet, the function falls back to the built-in curated
registry compiled into `flash.h`. In both cases the public name surface is the same
curated family.

## Parameters

None.

## Returns

`Note`
    A one-dimensional text payload listing the curated dataset names in load order.

    As of **Thursday, August 13, 2026**, the registry contains 13 names:

    - `tips`
    - `flights`
    - `petals`
    - `exams`
    - `retail_daily`
    - `weather_hourly`
    - `factor_blocks`
    - `sensor64`
    - `options_chain`
    - `customer_funnel`
    - `commute_modes`
    - `regimes`
    - `quality_batches`

## Examples

Example 1. Print the full curated registry.

```cpp
using namespace pluss::pl;

auto names = flash::get_dataset_names();
std::cout << "get_dataset_names() count=" << names.numel() << "\n";
for (std::int64_t i = 0; i < names.numel(); ++i) {
  std::cout << "  [" << i << "] " << names.at(i) << "\n";
}
```

Output:

```text
get_dataset_names() count=13
  [0] tips
  [1] flights
  [2] petals
  [3] exams
  [4] retail_daily
  [5] weather_hourly
  [6] factor_blocks
  [7] sensor64
  [8] options_chain
  [9] customer_funnel
  [10] commute_modes
  [11] regimes
  [12] quality_batches
```

Example 2. Check whether a particular curated dataset exists before loading it.

```cpp
using namespace pluss::pl;

auto names = flash::get_dataset_names();
bool has_options_chain = false;
for (std::int64_t i = 0; i < names.numel(); ++i) {
  if (names.at(i) == "options_chain") {
    has_options_chain = true;
    break;
  }
}
std::cout << "has options_chain=" << has_options_chain << "\n";
```

Output:

```text
has options_chain=1
```

## Notes

- Not `noexcept`.
- The returned names are curated and intentional. This is not a general filesystem
  browser over arbitrary CSV files.
- The list is ordered. That same order is reflected by the built-in registry and by the
  manifest-backed installed dataset family.
- Every name this function returns is accepted by
  [`flash::load_dataset`](load_dataset.md). Unknown names still return `std::nullopt`
  from `load_dataset`.

## See Also

- [`flash::load_dataset`](load_dataset.md): loads one curated dataset by name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="crayon_palette.html">
    <span class="label">Previous</span>
    <span class="title">crayon_palette</span>
  </a>
  <a class="next-link" href="load_dataset.html">
    <span class="label">Next</span>
    <span class="title">load_dataset</span>
  </a>
</div>
