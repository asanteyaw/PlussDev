# DataFrame::cast

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::cast(pluss::RangeView<CastSpec> specs) -> DataFrame

DataFrame::cast(pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes) -> DataFrame

DataFrame::cast(pluss::RangeView<std::pair<std::string_view, ThreadCastTarget>> feature_targets) -> DataFrame

DataFrame::cast(std::initializer_list<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes) -> DataFrame

DataFrame::cast(std::initializer_list<std::pair<std::string_view, ThreadCastTarget>> feature_targets) -> DataFrame
```

Cast one or more features to a different Pluss payload family or scalar dtype.

`DataFrame::cast(...)` is the table-level casting route when you want to change how selected columns participate in later operations without touching the rest of the frame.

## Parameters

**specs**

Structured cast instructions.

Use this overload when you need explicit control over:
- the target payload family,
- series dtype selection,
- parsing and invalid-value behavior,
- other cast-policy details carried by `CastSpec`.

**feature_dtypes**

Collection of `(feature_name, dtype)` pairs.

Use this overload when you simply want to recast existing features to concrete scalar dtypes such as `Int64`, `Bool`, or `Float64`.

## Returns

`DataFrame`
    New frame with the requested features cast and every untargeted feature preserved unchanged.

## See Also

- <a href="astype.html"><code>DataFrame::astype</code></a>: cast selected features directly to numeric series dtypes.
- <a href="../thread/cast.html"><code>Thread::cast</code></a>: cast one already-extracted thread.

## Notes

`feature_dtypes` is the compact route when you only need dtype conversion for existing features.

`specs` is the broader route when you want full target-family control, parse behavior, or more structured cast policy.

## Examples

Example 1. Cast several existing features by naming the target scalar dtypes directly.

```cpp
using namespace pluss::table;
using pluss::ScalarType;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed", pluss::make_series({1.0, 0.0, 1.0})},
    {"raw_name", pluss::make_note({"Ada", "Bola", "Chen"})},
}).title("raw");

auto out = raw.cast({
    {"student_id", ScalarType::Int64},
    {"passed", ScalarType::Bool},
});
```

### Output

```text
===================================
     | student_id  passed  raw_name
-----------------------------------
0    | 101         true    Ada     
1    | 102         false   Bola    
2    | 103         true    Chen    
-----------------------------------
[3 cs x 3 feat]
```

Example 2. Use a structured cast spec when you want the cast policy expressed explicitly.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed", pluss::make_series({1.0, 0.0, 1.0})},
    {"raw_name", pluss::make_note({"Ada", "Bola", "Chen"})},
}).title("raw");

DataFrame::CastSpec keep_note;
keep_note.feature = "raw_name";
keep_note.target = ThreadCastTarget::Note;

auto out = raw.cast({keep_note});
```

### Output

```text
===================================
     | student_id  passed  raw_name
-----------------------------------
0    | 101         1       Ada     
1    | 102         0       Bola    
2    | 103         1       Chen    
-----------------------------------
[3 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="caselbl_.html">
    <span class="label">Previous</span>
    <span class="title">caselbl_</span>
  </a>
  <a class="next-link" href="clip.html">
    <span class="label">Next</span>
    <span class="title">clip</span>
  </a>
</div>
