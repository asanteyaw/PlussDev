PAGE = dict(
    slug="ScalePolicy",
    summary="How an axis maps values: linear, log10 or symlog.",
    title="ScalePolicy",
    signature="""enum class ScaleKind : std::uint8_t { Linear, Log10, SymLog };

struct ScalePolicy {
  ScaleKind kind{ScaleKind::Linear};
  double base{10.0};
  double linear_threshold{1.0};

  static constexpr ScalePolicy linear() noexcept;
  static constexpr ScalePolicy log10() noexcept;
  static constexpr ScalePolicy symlog(double base_value = 10.0, double threshold = 1.0) noexcept;
};""",
    intro="""A `ScalePolicy` says how an axis maps data values to positions on the page. It is the argument of
[`Pane::scale_x`](../pane/scale_x.md), `scale_y` and `scale_z`. It is only a mapping: which part of the data is shown is decided
by the limits ([`LimitMethod`](LimitMethod.md) and `x_limits`), and the tick labels by [`TickFormat`](TickFormat.md).

Build one with the three factories rather than by filling the fields, because the factories set the fields that go with each kind:

- `ScalePolicy::linear()` is the default of every axis.
- `ScalePolicy::log10()` spaces values by their base-10 logarithm, so equal ratios take equal distances. Every plotted value, and
  any explicit limit, must be strictly positive.
- `ScalePolicy::symlog(base, threshold)` is linear within `[-threshold, threshold]` and logarithmic beyond it on both sides. It
  suits data that cross zero and also spread over many orders of magnitude, where `log10` cannot represent the zero or the
  negative values.""",
    table_title="## Fields",
    table="""| Field | Meaning | Default | Applies to |
|---|---|---|---|
| `kind` | `Linear`, `Log10` or `SymLog`. | `Linear` | all |
| `base` | The logarithm base. Must be finite and greater than 1 for `SymLog`. | `10` | `SymLog` (`Log10` is always base 10) |
| `linear_threshold` | Half the width of the linear region around zero. Must be finite and positive. | `1` | `SymLog` |""",
    sections=[("Validation", """Building a `ScalePolicy` never fails, even with nonsense `symlog` arguments. The check happens when the policy is passed to
`scale_x`, `scale_y` or `scale_z`: `base` must be finite and greater than 1, and `linear_threshold` finite and positive, or
`pluss::Error` is thrown. `log10` is also checked against the limits and the data: setting `log10` after limits that include zero or negative
values throws, and setting such limits after `log10` throws too, whichever order you use.""")],
    examples=[
        dict(id="gallery", text="The three kinds on the same data, which crosses zero and spans four orders of magnitude on the positive side. `Linear` crushes the small values at the bottom; `SymLog` keeps them readable on both sides of zero. (`Log10` cannot show this data, because it has non-positive values; it is shown in Example 2.)",
             code="""const std::vector<double> x{-4, -3, -2, -1, 0, 1, 2, 3, 4};
const std::vector<double> y{-9000, -800, -70, -6, 0, 6, 70, 800, 9000};

auto fig = studio("scale_policy_gallery");
fig.mosaic(1, 2);
auto& linear = fig.pane2("Linear");
linear.trend(x, y);
auto& symlog = fig.pane2("SymLog (base 10, threshold 1)");
symlog.trend(x, y);
symlog.scale_y(ScalePolicy::symlog());
fig.save_svg("figure.svg");"""),
        dict(id="log10", text="`log10` on positive data that grows by a constant ratio. On a log axis that is a straight line, which is why a log axis is the right one for rates of growth. The same data on a linear axis is an ever-steeper curve.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5, 6};
const std::vector<double> y{3, 9, 27, 81, 243, 729};

auto fig = studio("scale_policy_log10");
fig.mosaic(1, 2);
auto& linear = fig.pane2("Linear");
linear.trend(x, y);
auto& logarithmic = fig.pane2("Log10");
logarithmic.trend(x, y);
logarithmic.scale_y(ScalePolicy::log10());
fig.save_svg("figure.svg");"""),
        dict(id="threshold", text="The `symlog` threshold sets how much of the axis around zero is linear. A threshold of 1 keeps the points between -1 and 1 linear; a threshold of 100 keeps everything within 100 of zero linear, so the small values are spread out and only the large ones are compressed.",
             code="""const std::vector<double> x{-4, -3, -2, -1, 0, 1, 2, 3, 4};
const std::vector<double> y{-9000, -800, -70, -6, 0, 6, 70, 800, 9000};

auto fig = studio("scale_policy_threshold");
fig.mosaic(1, 3);
const double thresholds[] = {1.0, 10.0, 100.0};
for (int i = 0; i < 3; ++i) {
  auto& pane = fig.pane2("threshold " + std::to_string(static_cast<int>(thresholds[i])));
  pane.trend(x, y);
  pane.scale_y(ScalePolicy::symlog(10.0, thresholds[i]));
}
fig.save_svg("figure.svg");"""),
        dict(id="fields", text="The factories fill the fields in, and a policy can be read back. `symlog()` with no arguments is base 10 and threshold 1; `log10()` is `Log10` with base 10; a pane reports its policy through the axis it belongs to.",
             code="""const ScalePolicy a = ScalePolicy::linear();
const ScalePolicy b = ScalePolicy::log10();
const ScalePolicy c = ScalePolicy::symlog();
const ScalePolicy d = ScalePolicy::symlog(2.0, 0.5);
for (const ScalePolicy& p : {a, b, c, d})
  std::cout << "kind " << static_cast<int>(p.kind) << ", base " << p.base << ", linear_threshold " << p.linear_threshold << "\\n";

auto fig = studio("scale_policy_fields");
auto& pane = fig.pane2("Readback");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 10, 100});
pane.scale_y(ScalePolicy::log10());
std::cout << "y axis kind: " << static_cast<int>(pane.y_axis().scale.kind) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="errors", text="The refusals. `symlog` arguments are checked when the policy is applied, not when it is built; `log10` is refused after limits that include zero, and limits that include zero are refused after `log10`.",
             code="""auto fig = studio("scale_policy_errors");
auto& pane = fig.pane2("Errors");
pane.trend(std::vector<double>{1, 2}, std::vector<double>{1, 2});

try { pane.scale_y(ScalePolicy::symlog(1.0, 1.0)); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
try { pane.scale_y(ScalePolicy::symlog(10.0, 0.0)); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }

pane.y_limits(Range1d{-5.0, 5.0});
try { pane.scale_y(ScalePolicy::log10()); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }

auto other = studio("scale_policy_errors_2");
auto& p2 = other.pane2("Order");
p2.trend(std::vector<double>{1, 2}, std::vector<double>{1, 2});
p2.scale_y(ScalePolicy::log10());
try { p2.y_limits(Range1d{-5.0, 5.0}); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }"""),
    ],
    notes="""- A log axis cannot show zero or negative values. If the data can reach zero, use `symlog`, or shift or filter the data first.
- The grid follows the scale: on a log axis `MajorMinor` draws the decades and their subdivisions ([`GridStyle`](GridStyle.md)).
- The same three policies are available on the z axis of a 3D pane with `scale_z`.""",
    see=["[`Pane::scale_x`](../pane/scale_x.md), [`Pane::scale_y`](../pane/scale_y.md), [`Pane::scale_z`](../pane/scale_z.md): apply a policy.", "[`LimitMethod`](LimitMethod.md): how the range is fitted on top of the scale.", "[`TickFormat`](TickFormat.md): how the tick labels are written."],
)
