<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98118355ce18 -->

# DataFrame::case_study

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrameCaseStudyAccessor DataFrame::case_study() const
```

## What It Opens

Returns the frame's case-selection accessor. Calling `case_study()` alone does not
select, copy, or reorder data; it binds a lightweight handle to the current frame so
the following terminal can state how cases are identified.

The accessor separates four selection languages that should not be conflated:

- `at`, `slice`, and `take` identify physical positions;
- `label`, `labels`, and `span` identify values on the current case-label axis;
- `where` applies a Boolean decision per case;
- `pick` constrains named levels of a structured case axis.

Every terminal returns a `DataFrame`, including one-row and one-feature results. This
preserves frame shape, feature types, case labels, and ordinary chaining. Feature
selection deliberately remains on the returned frame:

```cpp
auto review = orders.case_study().take({2, 0, 2})
                    .features({"account", "amount"});
```

The [DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)
covers every terminal separately, including overloads, duplicate-label behavior,
negative positions, inclusive versus half-open bounds, mask alignment, structured
case levels, and failures.

## Choosing A Terminal

Use positions when row location itself is meaningful. Use labels when the case axis
is the durable identity. Use `where()` when the decision is already expressed as a
Boolean `Slate` or `Series`. Use `pick()` only after named levels have been created
with `label().set(...)`.

The closest root alternatives are intentionally narrower: `filter()` performs only
mask selection, `take()` performs only positional gather, and `features()` projects
only the feature axis. `case_study()` groups the complete case vocabulary without
overloading those root names further.

## Examples

Select one physical case and retain the result as a frame:

```cpp
using namespace pluss::table;
auto orders = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units", Slate(pluss::make_series({10, 7, 12}))},
});

auto last = orders.case_study().at(-1);
assert(last.shape() == std::pair<std::int64_t, std::int64_t>{1, 2});
```

Select by case label, retaining all duplicate matches:

```cpp
auto labeled = orders.label().set(
    Slate(pluss::make_note({"south", "coast", "coast"})));
auto coast = labeled.case_study().label("coast");
assert(coast.ncases() == 2);
```

Compose case selection with ordinary feature projection:

```cpp
auto large = orders.case_study().where(orders.features("units") >= 10)
                   .features({"city"});
assert(large.shape() == std::pair<std::int64_t, std::int64_t>{2, 1});
```

## Lifetime And Cost

The accessor stores a cheap owning frame handle, so it remains valid independently of
the expression that produced it. Accessor construction is constant-time. Allocation
and validation occur in the terminal operation; the original frame is never mutated.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bucketize.html"><span class="label">Previous</span><span class="title">bucketize</span></a>
  <a class="next-link" href="cast.html"><span class="label">Next</span><span class="title">cast</span></a>
</div>
