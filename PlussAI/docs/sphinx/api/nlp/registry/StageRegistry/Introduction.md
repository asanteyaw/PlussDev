# Using The Catalog

The catalog is read far more often than it is written. Built-in stages are always available, so most programs
never register anything. An application that defines its own stage types registers each one once, typically at
startup, before any workflow is loaded.

Registration returns whether it took effect, so a program can tell a first registration from a duplicate. The
catalog never replaces an existing entry, which keeps a built-in name from being hijacked by a later registration.

## See Also

- [`instance`](instance.md) -- the catalog itself.
- [`names`](names.md) -- lists what is registered.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
<!-- apiref:nav:end -->
