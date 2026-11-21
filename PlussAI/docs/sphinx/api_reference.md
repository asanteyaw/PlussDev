# API Reference

This section is organized by public family first, not by internal implementation
names or generated header dumps.

The goal is simple: enter through the family you are using in code, then move into
its handwritten reference pages without needing to know the private `node*` names
that sit underneath the public surface.

```{toctree}
:maxdepth: 4

api/plussai_root
```

## Conventions

- `pluss::RangeView<T>` in a signature means a read-only view of a sequence of `T`. It is the name this reference uses for
  such a parameter, not a type you can spell in your own code. To pass a sequence, write a braced list
  (`features({"a", "b"})`) or pass an existing `std::vector`.
