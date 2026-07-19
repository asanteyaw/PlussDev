# DataFrameNoteAccessor::chars

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::chars(std::int64_t start, std::optional<std::int64_t> stop = std::nullopt, std::int64_t step = 1) -> DataFrame
```

## What It Does

Extracts positional character slices from each note-backed text value in the current object.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

**step**
    Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"  Accra", "TEMA ", "Kumasi"}))},
    {"product", pluss::table::Thread(pluss::make_note({"Laptop", "desk", "Chair"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto notes = df.note();

auto out = notes.chars(1, 1, 1);
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
