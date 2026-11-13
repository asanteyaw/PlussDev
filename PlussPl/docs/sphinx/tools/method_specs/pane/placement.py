PAGE = dict(
    slug="placement",
    title="Pane::placement",
    signature="const PanePlacement& Pane::placement() const noexcept",
    intro="""Returns where the pane sits in the figure's grid, as a `PanePlacement`:

| Field | Meaning | Default |
|---|---|---|
| `row`, `col` | First cell of the pane, counted from 0 at the top left. | 0, 0 |
| `row_span`, `col_span` | How many rows and columns the pane covers. | 1, 1 |
| `explicit_slot` | `true` once the slot was chosen by [`place`](place.md) or by [`subpane2`](../studio/subpane2.md) / [`subpane3`](../studio/subpane3.md); `false` for a pane that is placed automatically. | false |
| `parent` | Index of the pane this one is nested in, or -1 for a top-level pane. | -1 |
| `local_rows`, `local_cols` | The shape of the nested grid inside `parent`; 0 for a top-level pane. | 0, 0 |
| `local_gutter_x`, `local_gutter_y` | The gaps of that nested grid. | 12, 12 |

The record is what was asked for, not a computed position. A pane that was never placed reports the defaults with `explicit_slot == false`, even though the renderer puts it in the next free cell of the [`mosaic`](../studio/mosaic.md); only an explicit placement is recorded, and it is validated for shape when it is made (non-negative cell, positive spans) and for fit when the figure is rendered. It reads the record and has no effect on the figure.""",
    parameters="*(none)*",
    returns="""`const PanePlacement&`
    The pane's own placement record.""",
    examples=[
        dict(id="default", text="A pane that was never placed. The record holds the defaults, `explicit_slot` is false, and the pane is a top-level pane with no nested grid.",
             code="""auto fig = studio("placement_default");
fig.mosaic(1, 2);
auto& pane = fig.pane2("automatic");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
const auto& p = pane.placement();
std::cout << "cell " << p.row << "," << p.col << " span " << p.row_span << "x" << p.col_span << ", explicit " << p.explicit_slot << "\\n";
std::cout << "parent " << p.parent << ", nested grid " << p.local_rows << "x" << p.local_cols << "\\n";
fig.pane2("second").trend(std::vector<double>{1, 2, 3}, std::vector<double>{3, 1, 2});
fig.save_svg("figure.svg");"""),
        dict(id="placed", text="After `place`. The record holds the requested cell and spans, and `explicit_slot` is true; `place(row, col)` is the one-cell form.",
             code="""auto fig = studio("placement_placed");
fig.mosaic(2, 2);
auto& wide = fig.pane2("wide");
wide.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
wide.place(0, 0, 1, 2);
std::cout << "wide: cell " << wide.placement().row << "," << wide.placement().col << " span " << wide.placement().row_span << "x" << wide.placement().col_span << ", explicit " << wide.placement().explicit_slot << "\\n";
auto& below = fig.pane2("below left");
below.trend(std::vector<double>{1, 2, 3}, std::vector<double>{3, 1, 2});
below.place(1, 0);
std::cout << "below: cell " << below.placement().row << "," << below.placement().col << " span " << below.placement().row_span << "x" << below.placement().col_span << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="subpane", text="A sub-pane. The record names the parent pane and the shape of the nested grid, and the cell is the cell inside that nested grid.",
             code="""auto fig = studio("placement_subpane");
auto& parent = fig.pane2("parent");
const std::size_t parent_index = fig.pane_index(parent);
auto& child = fig.subpane2(parent_index, 2, 3, 1, 2, 1, 1, "child");
child.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
const auto& p = child.placement();
std::cout << "parent " << p.parent << ", nested grid " << p.local_rows << "x" << p.local_cols << ", cell " << p.row << "," << p.col << ", explicit " << p.explicit_slot << "\\n";
std::cout << "gutters " << p.local_gutter_x << ", " << p.local_gutter_y << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="fit_checked_later", text="Shape is checked when placing, fit when rendering. A cell outside the grid is accepted by `place` and recorded; rendering the figure reports that it does not fit.",
             code="""auto fig = studio("placement_fit");
fig.mosaic(1, 2);
auto& pane = fig.pane2("outside");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
pane.place(3, 3);
std::cout << "recorded: " << pane.placement().row << "," << pane.placement().col << "\\n";
try {
  fig.to_svg();
} catch (const pluss::Error& e) {
  std::cout << "render: " << e.message() << "\\n";
}
try {
  pane.place(-1, 0);
} catch (const pluss::Error& e) {
  std::cout << "shape: " << e.message() << "\\n";
}
auto ok = studio("placement_fit_ok");
ok.mosaic(1, 2);
ok.pane2("a").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
ok.save_svg("figure.svg");"""),
    ],
    notes="""- `noexcept`, and has no effect on the figure. The reference stays valid for the life of the pane.
- `explicit_slot` is the way to tell a placed pane from one the renderer placed.""",
    see=["[`Pane::place`](place.md): chooses the cell.", "[`Studio::mosaic`](../studio/mosaic.md): the grid.", "[`Studio::subpane2`](../studio/subpane2.md), [`subpane3`](../studio/subpane3.md): nested grids.", "[`Studio::mosaic_layout`](../studio/mosaic_layout.md): the grid description."],
)
