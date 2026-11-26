<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 273d577926cc -->

# Axel::show

[Back to Axel API Reference](../../axel_api_reference)

## Signatures

```cpp
void Axel::show(std::ostream& os) const
void Axel::show() const
```

## What It Does

The `std::ostream&` overload writes the axel's summary: a short list of facts that stays the
same length however many registries it stacks or however many fields they have — how deep it
is, the range of fields per registry and of states per field, whether the field names are the
same in every registry, a tally of what the slots hold, and the depth labels. It is the same
text `operator<<` writes. [`to_string()`](to_string.md) is different: it is a JSON dump.

The no-argument overload opens the axel in the PlussView window: one registry at a time, as
the ragged grid a [`Registry`](../registry/show.md) opens, with a **Level** menu (First,
Previous, Next, Last, Go to…) to move between registries. Inside a PlussII notebook it hands the
axel to the notebook as the same summary instead of opening a window.

## Parameters

**os** (`std::ostream&`, stream overload)

The stream to write the summary to.

## Returns

`void`

## Examples

Example 1. Print an axel's summary.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
});

axel.show(std::cout);
```

### Output

```text
========================================================================================================================
Axel
------------------------------------------------------------------------------------------------------------------------
fact             | value
------------------------------------------------------------------------------------------------------------------------
depth            | 2 registries
fields           | 1 each
states per field | 1 to 2 (3 in all)
field names      | the same in every registry
ptypes           | Series × 2
depth labels     | none
========================================================================================================================
```

Example 2. `show(os)` writes the same text as `operator<<`.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
});

std::ostringstream via_show;
axel.show(via_show);

std::ostringstream via_operator;
via_operator << axel;

std::cout << std::boolalpha << (via_show.str() == via_operator.str());
```

### Output

```text
true
```

## Notes

- The summary never lists field names or states, so it is as short for an axel of hundreds of
  registries as for one of two. To see a registry, take it with `axel.registry(i)`, or open the
  window with `show()`.

## See Also

- [`Axel::to_string`](to_string.md): a JSON dump of the axel.
- [`Archive::show`](../archive/show.md): the same method for a stack of catalogs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
