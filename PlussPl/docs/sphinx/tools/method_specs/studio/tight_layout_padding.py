PAGE = dict(
    slug="tight_layout_padding",
    title="Studio::tight_layout_padding",
    signature="double Studio::tight_layout_padding() const noexcept",
    intro="""Returns the padding of the compact layout, in drawing units: 6 for a new figure, and the `padding` argument of the last [`tight_layout`](tight_layout.md) call (a call with no padding argument resets it to 6). The padding is the extra room added back around the edges when the compact layout is on; it is stored even while the layout is off ([`tight_layout_enabled`](tight_layout_enabled.md) tells which), and it has no effect on the figure until the layout is switched on.

The value is always finite and not negative, because `tight_layout` refuses anything else and leaves the previous value in place.""",
    parameters="*(none)*",
    returns="""`double`
    The padding, 6 by default.""",
    examples=[
        dict(id="values", text="The default and each call. A new figure reports 6; `tight_layout(true, p)` stores `p`, and a call without a padding resets it to 6.",
             code="""auto fig = studio("tight_padding_values");
std::cout << "new figure: " << fig.tight_layout_padding() << "\\n";
fig.tight_layout(true, 14.0);
std::cout << "tight_layout(true, 14): " << fig.tight_layout_padding() << "\\n";
fig.tight_layout();
std::cout << "tight_layout(): " << fig.tight_layout_padding() << "\\n";
fig.tight_layout(true, 0.0);
std::cout << "zero is allowed: " << fig.tight_layout_padding() << "\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="effect", text="The padding changes the picture once the layout is on. Three paddings give three different figures, and a larger padding leaves more room around the plot.",
             code="""auto build = [](double padding) {
  auto fig = studio("tight_padding_effect");
  fig.tight_layout(true, padding);
  fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  return fig.to_svg();
};
std::cout << "0 vs 6: " << (build(0.0) != build(6.0)) << ", 6 vs 24: " << (build(6.0) != build(24.0)) << "\\n";
auto fig = studio("tight_padding_effect_out");
fig.tight_layout(true, 24.0);
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="no_effect_off", text="No effect while the layout is off. A padding stored with the layout off leaves the figure exactly as it was.",
             code="""auto build = [](bool store_padding) {
  auto fig = studio("tight_padding_off");
  if (store_padding) fig.tight_layout(false, 30.0);
  fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  return fig.to_svg();
};
std::cout << "padding stored, layout off == untouched figure: " << (build(true) == build(false)) << "\\n";
auto fig = studio("tight_padding_off_out");
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Invalid paddings are refused and the stored value is kept: a negative number, infinity and NaN each throw `pluss::Error`.",
             code="""auto fig = studio("tight_padding_invalid");
fig.tight_layout(true, 9.0);
const double inf = std::numeric_limits<double>::infinity();
for (double bad : {-1.0, inf, std::numeric_limits<double>::quiet_NaN()}) {
  try {
    fig.tight_layout(true, bad);
  } catch (const pluss::Error& e) {
    std::cout << "padding " << bad << ": " << e.message() << "\\n";
  }
}
std::cout << "still " << fig.tight_layout_padding() << ", enabled: " << fig.tight_layout_enabled() << "\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `noexcept`, and has no effect on the figure.
- Padding is in drawing units, the same units as [`size`](size.md).""",
    see=["[`Studio::tight_layout`](tight_layout.md): sets the padding.", "[`Studio::tight_layout_enabled`](tight_layout_enabled.md): whether the compact layout is on."],
)
