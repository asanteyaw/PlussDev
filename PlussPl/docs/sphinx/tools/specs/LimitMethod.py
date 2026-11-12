PAGE = dict(
    slug="LimitMethod",
    summary="How an axis range is fitted to the data.",
    title="LimitMethod",
    signature="""enum class LimitMode : std::uint8_t { AutoFit, Manual };
enum class LimitMethod : std::uint8_t { TickAligned, Tight, Padded };

struct Range1d {
  double low{0.0};
  double high{1.0};
};""",
    intro="""An axis range is either **fitted automatically** to the data or set by hand. `LimitMethod` chooses how the automatic fit is done; it is the
argument of `Pane::x_limit_method`, `y_limit_method`, `z_limit_method` and `color_limit_method`. `LimitMode` says which of the two is in
force: `AutoFit` until `x_limits`, `y_limits`, `z_limits` or `color_limits` set a `Range1d` by hand (after which the mode is `Manual`),
and `AutoFit` again after `auto_x`, `auto_y`, `auto_z` or `auto_color`.

The method matters only in `AutoFit` mode. Once a range is set by hand the axis uses exactly that range, and the method is not consulted, whatever it was set to and in whichever order the calls were made.""",
    table="""| Value | The fitted range | Take it when |
|---|---|---|
| `TickAligned` | The data's range widened to round tick values, so the axis starts and ends on labelled ticks. This is the default. | A conventional, readable axis. |
| `Tight` | The data's range as it is, with no margin. | The data should fill the plot area, or the axis ends must be the data's extremes. |
| `Padded` | The data's range with a proportional margin on both sides. | Points should stay clear of the frame, without the axis landing on round values. |""",
    examples=[
        dict(id="gallery", text="The three methods on the same awkward data (2.3 to 6.2). `TickAligned` ends the axis on 2 and 7 (round tick values); `Tight` ends it on 2.3 and 6.2 so the line touches the frame; `Padded` leaves a margin either side of the data.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5};
const std::vector<double> y{2.3, 4.7, 3.1, 5.85, 6.2};
const LimitMethod methods[] = {LimitMethod::TickAligned, LimitMethod::Tight, LimitMethod::Padded};
const char* names[] = {"TickAligned", "Tight", "Padded"};

auto fig = studio("limit_method_gallery");
fig.mosaic(1, 3);
for (int i = 0; i < 3; ++i) {
  auto& pane = fig.pane2(names[i]);
  pane.trend(x, y);
  pane.y_limit_method(methods[i]);
}
fig.save_svg("figure.svg");"""),
        dict(id="manual", text="A manual range overrides the method. After `y_limits` the two figures below, which differ only in the method, are byte for byte identical, and the axis runs exactly from 0 to 10.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5};
const std::vector<double> y{2.3, 4.7, 3.1, 5.85, 6.2};
std::string svg[2];
const LimitMethod methods[] = {LimitMethod::Tight, LimitMethod::Padded};
for (int i = 0; i < 2; ++i) {
  auto fig = studio("limit_method_manual");
  auto& pane = fig.pane2("Manual 0 to 10");
  pane.trend(x, y);
  pane.y_limits(Range1d{0.0, 10.0});
  pane.y_limit_method(methods[i]);
  svg[i] = fig.to_svg();
  if (i == 0) fig.save_svg("figure.svg");
}
std::cout << "Tight == Padded with manual limits: " << (svg[0] == svg[1]) << "\\n";"""),
        dict(id="auto_again", text="`auto_y` returns an axis to automatic fitting, and the method it had before applies again. The limits are manual after `y_limits`, automatic again after `auto_y`.",
             code="""auto fig = studio("limit_method_auto_again");
auto& pane = fig.pane2("Back to auto");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
pane.y_limit_method(LimitMethod::Tight);
std::cout << "mode at the start: " << static_cast<int>(pane.y_axis().limit_mode) << "\\n";
pane.y_limits(Range1d{0.0, 10.0});
std::cout << "after y_limits:    " << static_cast<int>(pane.y_axis().limit_mode) << "\\n";
pane.auto_y();
std::cout << "after auto_y:      " << static_cast<int>(pane.y_axis().limit_mode) << "\\n";
std::cout << "method is still Tight: " << (pane.y_axis().limit_method == LimitMethod::Tight) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="color", text="The colour axis has a method too. A `cloud` coloured by a series takes its colour range from the data, here with a constant marker size, and `Tight` makes the extremes of the data the ends of the colour scale.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5, 6};
const std::vector<double> y{1, 3, 2, 5, 4, 6};
const std::vector<double> c{2.3, 4.7, 3.1, 5.85, 6.2, 3.9};
const LimitMethod methods[] = {LimitMethod::TickAligned, LimitMethod::Tight};
const char* names[] = {"TickAligned colour range", "Tight colour range"};

auto fig = studio("limit_method_color");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane2(names[i]);
  pane.cloud(x, y, std::vector<double>(6, 8.0), c);
  pane.color_limit_method(methods[i]);
  std::cout << names[i] << ": colour range in force " << static_cast<int>(pane.color_axis().limit_method) << "\\n";
}
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `LimitMethod` and `LimitMode` describe an axis that holds the limits; they are read through `Pane::x_axis()`, `y_axis()`, `z_axis()` and `color_axis()`.
- Fitting happens when the figure is rendered, so the order in which you call `y_limit_method` and `trend` does not matter.
- A log axis fits its range in the logarithm of the data, then applies the method ([`ScalePolicy`](ScalePolicy.md)).""",
    see=["[`Pane::x_limit_method`](../pane/x_limit_method.md), [`Pane::y_limit_method`](../pane/y_limit_method.md): set the method.", "[`Pane::x_limits`](../pane/x_limits.md), `Pane::auto_x`: the manual range and the way back.", "[`ScalePolicy`](ScalePolicy.md), [`TickFormat`](TickFormat.md): the scale and the labels."],
)
