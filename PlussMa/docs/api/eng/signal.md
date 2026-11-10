<!-- generated from signal.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Signal

The `pluss::ma::eng::signal` family supplies Series-native tools for shaping, filtering, correlating, and analyzing sampled signals. It keeps coefficient construction, filter execution, and recursive-filter design separate so each stage can be inspected and reused.

## Branches

Use [Signal windows](signal/windows.md) to generate finite tapers for spectral analysis and FIR construction. Use [Filtering and response](signal/filtering.md) to convolve, correlate, execute direct-form or second-order-section filters, remove phase delay, and inspect frequency response. Use [IIR design](signal/design.md) to create Butterworth and Chebyshev filters from engineering specifications.

All branches accept or return `Series`, preserving dtype, platform, batching, and differentiation behavior. Coefficient creation is therefore not a detached host-side utility: generated filters can remain on the same execution platform as the data they process.

```{toctree}
:maxdepth: 3
:hidden:

signal/windows
signal/filtering
signal/design
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

signal/windows
signal/filtering
signal/design
```
<!-- apiref:toctree:end -->
