<!-- pluss-hand-authored -->

# Studio::pane_index

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
[[nodiscard]] std::size_t Studio::pane_index(const Pane& pane) const
```

## What It Does

Returns the 0-indexed position of `pane` within this figure's pane list — the same index
used by [`subpane2()`](subpane2.md)/[`subpane3()`](subpane3.md)'s `parent_index` parameter,
and the position `pane` would appear at in [`panes()`](panes.md). Internally, the lookup is
by **address**: it scans the figure's panes for one whose address matches `&pane`.

```{important} pane must currently belong to THIS studio, by address
If no pane in this figure's list has the same address as the passed-in `pane`, this throws
`std::invalid_argument("pane_index: pane does not belong to this studio")`. This covers two
distinct cases: a `Pane` built standalone and never added to any figure (see Example 3),
and a `Pane` that belongs to a *different* `Studio` (see Example 4).
```

```{danger} A stale Pane& (see pane2()'s important callout) makes this fail unpredictably
Because the lookup is address-based, calling `pane_index()` with a `Pane&` that was
invalidated by a later `pane2()`/`pane3()`/`subpane2()`/`subpane3()` call is undefined
behavior — in practice, this was observed to make `pane_index()` throw its
"does not belong to this studio" message for a pane that *was* genuinely added to the
figure, simply because the stale reference's address no longer matched anything live in
the vector. This is not a guaranteed, reliable error message to depend on — it's
evidence of real, observed damage from the exact hazard described in
[`pane2()`](pane2.md)'s important callout, not documented behavior. The fix is the same:
call `pane_index()` **immediately** after creating the pane, before creating any others —
see Example 1 and Example 2, which follow that safe ordering.
```

## Parameters

**pane** (`const Pane&`)
    The pane to look up. Must be a currently-live reference to a pane stored in this
    figure — see the callouts above.

## Returns

`std::size_t`
    The 0-indexed position of `pane` in this figure's pane list.

## Examples

Example 1 and 2. Two panes, each looked up **immediately** after creation, before the next
`pane2()` call — the only safe ordering.

```cpp
using namespace pluss::pl;

auto fig = studio("pane_index_check");
auto& first = fig.pane2("first");
flash::lineplot(first, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});
std::size_t first_index = fig.pane_index(first);

auto& second = fig.pane2("second");
flash::lineplot(second, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{3.0, 5.0, 4.0});
std::size_t second_index = fig.pane_index(second);

fig.show();

std::cout << "index of first: " << first_index << "\n";
std::cout << "index of second: " << second_index << "\n";
```

### Output

```text
index of first: 0
index of second: 1
```

Example 3. A `Pane` built standalone, never added to any `Studio` — throws.

```cpp
using namespace pluss::pl;

auto fig = studio("pane_index_orphan");
auto& real_pane = fig.pane2();
flash::lineplot(real_pane, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});

Pane orphan(PaneKind::Planar, Theme::paper());
try {
  fig.pane_index(orphan);
} catch (const std::invalid_argument& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
pane_index: pane does not belong to this studio
```

Example 4. A `Pane` that belongs to a **different** `Studio` — also throws, for the same
reason (its address isn't in *this* figure's pane list).

```cpp
using namespace pluss::pl;

auto fig = studio("pane_index_cross_studio");
auto& real_pane = fig.pane2();
flash::lineplot(real_pane, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});

auto other_fig = studio("other_studio");
auto& other_pane = other_fig.pane2("belongs elsewhere");

try {
  fig.pane_index(other_pane);
} catch (const std::invalid_argument& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
pane_index: pane does not belong to this studio
```

## Notes

- `[[nodiscard]]` — discarding the return value is a compiler warning.
- Not `noexcept` — throws `std::invalid_argument` when the lookup fails.
- The lookup is by address, not by value/content — two structurally-identical panes are
  still distinguished by which one is actually stored in this figure.
- Returns the *current* index; if this figure's panes were ever removed (there is no public
  API to do so today) indices could shift, but as of this API surface, indices are stable
  for the lifetime of the figure once assigned.
- See the danger callout above: never call this with a `Pane&` obtained before a later
  pane-creation call — capture the index immediately instead.

## See Also

- [`Studio::pane2`](pane2.md) / [`pane3`](pane3.md): create the panes this looks up, and
  document the reference-invalidation hazard in full.
- [`Studio::subpane2`](subpane2.md) / [`subpane3`](subpane3.md): consume this same index as
  their `parent_index` parameter.
- [`Studio::panes`](panes.md): the full pane list this indexes into.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pane3.html">
    <span class="label">Previous</span>
    <span class="title">pane3</span>
  </a>
  <a class="next-link" href="subpane2.html">
    <span class="label">Next</span>
    <span class="title">subpane2</span>
  </a>
</div>
