<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d9c289d12321 -->

# DataFrame::select

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame::select(
    pluss::msharp::RangeView::Of<Slate> conditions,
    pluss::msharp::RangeView::Of<DataFrame> choices,
    const DataFrame& default_value) -> DataFrame

static DataFrame::select(
    std::initializer_list<Slate> conditions,
    std::initializer_list<DataFrame> choices,
    const DataFrame& default_value) -> DataFrame
```

## What It Does

A static factory that builds a new frame by choosing, case by case, which candidate
frame's row to use — the row-wise analogue of a chained if/else-if/else across several
whole `DataFrame`s at once. For every case (row), `select()` walks `conditions` **in
order** and uses the row from the first candidate `choices[i]` whose `conditions[i]`
is `true` for that case; if none of the conditions are `true` for a case, that case's
row comes from `default_value` instead. The same set of conditions is applied
identically across every feature, so a single call reshuffles an entire frame's worth
of columns in one pass rather than one feature at a time.

This is not an instance method — there is no frame to call it "on". You supply
`conditions`, `choices`, and `default_value` up front, and `select()` assembles the
result from scratch. Because of this, `select()` was previously missing from this
reference entirely; the header parser that catalogs `DataFrame`'s methods didn't
recognize this static, `RangeView`-based factory as a documentable method the way it
recognizes ordinary `const` member functions. That gap is fixed here, but it's worth
flagging as a real coverage hole in the automated tooling, not just a documentation
oversight.

```{important} Priority is positional: the first true condition wins
If more than one condition is `true` for the same case, `conditions[0]`'s choice wins
— `select()` does not use the *last* matching condition, and it does not merge or
average competing choices. See Example 2 for a worked demonstration where deliberately
overlapping conditions produce different results depending on which one is listed
first.
```

## Parameters

**conditions** (`RangeView::Of<Slate>` or `std::initializer_list<Slate>`)

An ordered, non-empty list of boolean masks, one per candidate in `choices`. Each
`Slate` must be a `Series`-kind, `Bool`-scalar-type slate with one value per case —
the same requirement `land()`/`lor()`/`lxor()` place on their operands (see
[`land()`](land.md)). `conditions` and `choices` must have the same length; each
position pairs one condition with the candidate frame it "unlocks."

**choices** (`RangeView::Of<DataFrame>` or `std::initializer_list<DataFrame>`)

One candidate frame per condition, evaluated in the same order as `conditions`. Every
choice must **exactly** match `default_value`'s case count, feature count, feature
names (in the same order), case labels, and structured case-axis metadata — `select()`
does not align or reorder candidates for you first. Only the participating *cell
values* are allowed to differ between candidates.

**default_value** (`const DataFrame&`)

The frame supplying both the fallback row for any case where no condition is `true`,
and the structural template (feature names, order, case labels, case-axis metadata)
that every entry in `choices` is checked against.

## Returns

`DataFrame`
    A new frame with `default_value`'s structure (feature names and order, case
    labels, title, case-axis metadata) and, for every case, the cell values pulled
    from the first candidate in `choices` whose matching condition is `true` — or from
    `default_value` itself if no condition matched that case.

## Examples

Example 1. Build a three-tier billing frame from a usage metric: a `true`
`conditions[0]` picks the premium rate, `conditions[1]` picks the standard rate, and
cases matching neither fall through to the basic (default) rate.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"usage", pluss::make_series({10.0, 250.0, 80.0, 500.0, 30.0})},
}, pluss::make_note({"c1", "c2", "c3", "c4", "c5"}));

std::cout << customers;
```

### Output (customers)

```text
============
     | usage
------------
c1   | 10   
c2   | 250  
c3   | 80   
c4   | 500  
c5   | 30   
------------
[5 cs x 1 feat]
```

```cpp
auto tier_high = Slate(pluss::make_series(
    {false, true, false, true, false}, pluss::ScalarType::Bool));
auto tier_mid = Slate(pluss::make_series(
    {false, true, true, true, false}, pluss::ScalarType::Bool));

auto premium = DataFrame({
    {"rate", pluss::make_series({0.05, 0.05, 0.05, 0.05, 0.05})},
    {"fee",  pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0})},
}, pluss::make_note({"c1", "c2", "c3", "c4", "c5"}));

auto standard = DataFrame({
    {"rate", pluss::make_series({0.08, 0.08, 0.08, 0.08, 0.08})},
    {"fee",  pluss::make_series({2.0, 2.0, 2.0, 2.0, 2.0})},
}, pluss::make_note({"c1", "c2", "c3", "c4", "c5"}));

auto basic = DataFrame({
    {"rate", pluss::make_series({0.12, 0.12, 0.12, 0.12, 0.12})},
    {"fee",  pluss::make_series({5.0, 5.0, 5.0, 5.0, 5.0})},
}, pluss::make_note({"c1", "c2", "c3", "c4", "c5"}));

auto billing = DataFrame::select({tier_high, tier_mid}, {premium, standard}, basic);
std::cout << billing;
```

### Output (billing)

```text
=================
     | rate  fee 
-----------------
c1   | 0.12  5   
c2   | 0.05  0   
c3   | 0.08  2   
c4   | 0.05  0   
c5   | 0.12  5   
-----------------
[5 cs x 2 feat]
```

`c2` and `c4` clear the `tier_high` threshold (`250` and `500` usage), so they pick up
`premium`'s `rate`/`fee`. `c3` (`80` usage) misses `tier_high` but clears `tier_mid`,
landing on `standard`. `c1` and `c5` (`10` and `30` usage) miss both conditions and
fall through to `basic`, the `default_value`. Note both features move together per
case — `select()` doesn't pick `rate` from one candidate and `fee` from another.

Example 2. Overlapping conditions: when more than one condition is `true` for the same
case, the **first** one listed wins — reordering the arguments changes the result.

```cpp
auto cond_a = Slate(pluss::make_series({true, true}, pluss::ScalarType::Bool));
auto cond_b = Slate(pluss::make_series({true, false}, pluss::ScalarType::Bool));

auto choice_a = DataFrame({
    {"tag", pluss::make_note({"A", "A"})},
}, pluss::make_note({"c1", "c2"}));

auto choice_b = DataFrame({
    {"tag", pluss::make_note({"B", "B"})},
}, pluss::make_note({"c1", "c2"}));

auto fallback = DataFrame({
    {"tag", pluss::make_note({"D", "D"})},
}, pluss::make_note({"c1", "c2"}));

auto a_first = DataFrame::select({cond_a, cond_b}, {choice_a, choice_b}, fallback);
std::cout << a_first;
```

### Output (`cond_a` listed first)

```text
===========
     | tag 
-----------
c1   | A   
c2   | A   
-----------
[2 cs x 1 feat]
```

```cpp
auto b_first = DataFrame::select({cond_b, cond_a}, {choice_b, choice_a}, fallback);
std::cout << b_first;
```

### Output (`cond_b` listed first)

```text
===========
     | tag 
-----------
c1   | B   
c2   | A   
-----------
[2 cs x 1 feat]
```

`c1` satisfies both `cond_a` and `cond_b` in both calls — the *only* thing that changed
between the two calls is which condition/choice pair comes first in the list. When
`cond_a` leads, `c1` gets `"A"`; when `cond_b` leads, the same case gets `"B"` instead.
`c2` only ever satisfies `cond_a`, so it lands on `"A"` regardless of ordering.

Example 3. A winning candidate's own missing values pass straight through — `select()`
does not treat a chosen cell's `NULL` as a reason to fall back to a lower-priority
candidate or the default.

```cpp
auto cond = Slate(pluss::make_series({true, false}, pluss::ScalarType::Bool));

// "c1"'s label is explicitly missing: an empty-string placeholder paired with an
// invalid entry in the companion validity mask, the same idiom used whenever a Note
// needs an explicit NULL cell (see count.md).
auto label_valid = pluss::make_series({false, true}, pluss::ScalarType::Bool);
auto choice = DataFrame({
    {"label", Slate(pluss::make_note({"", "y"}), label_valid)},
}, pluss::make_note({"c1", "c2"}));

auto fallback = DataFrame({
    {"label", pluss::make_note({"def0", "def1"})},
}, pluss::make_note({"c1", "c2"}));

auto out = DataFrame::select({cond}, {choice}, fallback);
std::cout << out;
```

### Output

```text
============
     | label
------------
c1   | NULL 
c2   | def1 
------------
[2 cs x 1 feat]
```

`c1` satisfies `cond`, so `choice`'s row wins for that case — but `choice`'s `"c1"`
value is itself missing, so the result is `NULL`, not a silent fallback to
`fallback`'s `"def0"`. `c2` never satisfies `cond`, so it takes `fallback`'s `"def1"`
as expected.

Example 4. The result is an ordinary `DataFrame` — keep working with it exactly like
any other frame.

```cpp
auto rounded_billing = billing.round(1);
std::cout << rounded_billing;
```

### Output

```text
=================
     | rate  fee 
-----------------
c1   | 0.1   5   
c2   | 0.1   0   
c3   | 0.1   2   
c4   | 0.1   0   
c5   | 0.1   5   
-----------------
[5 cs x 2 feat]
```

`select()`'s output from Example 1 chains straight into [`round()`](round.md) — or
[`assign()`](assign.md), [`save_csv()`](save_csv.md), or anything else that takes a
`DataFrame` — with no special handling required.

## Notes

- Unlike the `Slate`-level `Slate::select()` this delegates to internally, the
  `DataFrame`-level `select()` does **not** support scalar-like (single-row)
  `choices` or `default_value` — every choice's `ncases()` must exactly match
  `default_value.ncases()`. Broadcasting a single-row candidate across every case is a
  `Slate::select()`-only capability.
- Every `choices` entry is checked against `default_value` for feature names (in
  order), case count, case labels, and case-axis metadata, and throws if any of them
  disagree — quoting the exact checks: *"all choices must match default feature
  order"*, *"all choices must match default case labels"*, and *"all choices must
  match default structured case metadata"*. Only cell values may differ between
  candidates.
- `conditions` and `choices` must have the same length, and at least one condition is
  required — an empty `conditions` list throws (*"at least one condition is
  required"*).
- Each condition `Slate` must be a `Bool`-typed `Series` — the same constraint
  documented on [`land()`](land.md). A numeric 0/1 column is rejected, not treated as
  truthy.
- The same condition list drives every feature identically — there is no per-feature
  condition list the way [`add()`](add.md)'s `Slate` overload has an `axis` choice.

```{caution} A generator/coverage gap, not just a documentation gap
`select()` had no generated page at all prior to this entry — every other
`DataFrame` method, even undocumented ones, at least had an auto-generated stub. If
you add other static, `RangeView`-based factory methods to `DataFrame` in the future,
verify `generate_api_reference.py` actually discovers them before assuming a missing
page means the method doesn't exist.
```

## See Also

- [`DataFrame::land`](land.md): the `Bool`-only constraint `conditions` shares with the
  logical method family.
- [`DataFrame::where`](where.md): a per-cell conditional fill against a single
  condition, rather than a row-wise pick across several whole candidate frames.
- [`DataFrame::assign`](assign.md): attach the result of a `select()` call as a new
  column on an existing frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="se.html">
    <span class="label">Previous</span>
    <span class="title">se</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
