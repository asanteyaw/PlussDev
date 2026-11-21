# API Reference

This section is organized by public object first, not by whatever file happened to generate the page.

The goal is simple: you should be able to enter through the object you are holding in code, then move method-by-method through a reference page that explains signatures, parameters, overload differences, examples, and output clearly enough to use the API without guessing.

```{toctree}
:maxdepth: 2

api/plussmic_root
```

## Conventions

- `pluss::RangeView<T>` in a signature means a read-only view of a sequence of `T`. It is the name this reference uses for
  such a parameter, not a type you can spell in your own code. To pass a sequence, write a braced list
  (`features({"a", "b"})`) or pass an existing `std::vector`.
