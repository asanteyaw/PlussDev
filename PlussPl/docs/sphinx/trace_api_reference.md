# Trace API Reference

This page documents the full public `pluss::pl::Trace` surface — the object representing
one drawn series/element inside a [`Pane`](pane_api_reference.md). Every `Pane`
trace-creation method ([`trend`](api/pane/trend.md), [`cloud`](api/pane/cloud.md),
[`sheet`](api/pane/sheet.md), and so on) returns a `Trace&`, which this reference's 11
methods use to control that one trace's styling (`label`, `width`, `stroke`, `palette`,
`color_limits`, `shade`, `lighting`, `material`, `transparent`, `annotate_levels`) and
visibility (`visible`/`hide`/`show`), independently of every other trace in the pane.

`Trace`'s method surface is far smaller than `Pane`'s (13 pages vs. 108) — a `Trace` has no
axes, no legend placement, no layout of its own; all of that lives one level up on the
`Pane` that owns it.

```{important} Pane::traces_ is also a std::vector -- the same reference-invalidation rule applies
Pane stores its traces in a single std::vector<Trace> internally, exactly like
Studio stores its panes in std::vector<Pane>. A Trace& returned by any
trace-creation method is invalidated the moment ANOTHER trace-creation call is
made on the SAME Pane -- not just by pane-creation calls on the enclosing
Studio. Read or capture anything you need from a Trace& immediately after
creating it, before creating the next trace on that pane. See
Trace::kind for a worked, directly-verified demonstration of this exact hazard.
```

```{important} Fixed: transparent()/annotate_levels() no longer default enable
Earlier revisions declared transparent()/annotate_levels() setters as
(bool enable = true), sharing their names with separate const getters -- a
real overload-resolution hazard where a bare t.transparent() on a non-const
Trace& silently resolved to the SETTER, directly confirmed by compiled
reproduction during this documentation effort. Both have since been fixed at
the source: enable is now required, so a bare zero-argument call always
resolves to the getter. The same fix was applied to Pane::visible,
Pane::axes_visible, and Pane::parallel_xy_labels, which carried the identical
pattern. Trace::visible() never had this problem in the first place -- it was
deliberately designed with hide()/show() as separate setters instead of a
same-named bool overload -- see Trace::visible for the contrast.
```

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.

## Method Pages

```{toctree}
:hidden:
:maxdepth: 1

api/trace/annotate_levels
api/trace/color_limits
api/trace/kind
api/trace/label
api/trace/lighting
api/trace/material
api/trace/palette
api/trace/payload
api/trace/shade
api/trace/stroke
api/trace/transparent
api/trace/visible
api/trace/width
```
