PAGE = dict(
    slug="TickFormat",
    summary="How tick labels are written.",
    title="TickFormat",
    signature="enum class TickFormat : std::uint8_t { Auto, Plain, Fixed3, Fixed6, Scientific, Engineering, Integer, Percent };",
    intro="""`TickFormat` chooses how the numbers on an axis are written. It is the argument of `Pane::x_tick_format`, `y_tick_format` and
`z_tick_format`, and the value is stored with the axis (`Pane::x_axis().tick_format`). It changes the text of the tick labels and nothing else: the positions of the ticks, the limits
([`LimitMethod`](LimitMethod.md)) and the scale ([`ScalePolicy`](ScalePolicy.md)) are not affected.

`Auto` lets the renderer pick a compact form from the data, and is right for most figures. Pick a fixed format when a figure must
use the same notation as its text, when several panes must be compared, or when the automatic choice is too terse or too long.
For `Auto` alone, `x_auto_tick_decimals`, `y_auto_tick_decimals` and `z_auto_tick_decimals` set how many decimals the automatic form may use.""",
    table="""| Value | Written as | Take it when |
|---|---|---|
| `Auto` | A compact form chosen from the data: `900000`, `0.09`. This is the default. | You have no requirement. |
| `Plain` | The number in ordinary positional notation: `900000`, `0.09`. It never switches to a power of ten. | The labels must stay in positional notation whatever the range. |
| `Fixed3` | Three decimals: `0.250`. | Several panes share a scale and must align. |
| `Fixed6` | Six decimals: `0.250000`. | Small differences matter. |
| `Scientific` | A mantissa with two decimals and a signed power of ten: `9.00e+05`. | A very wide range of magnitudes. |
| `Engineering` | A mantissa with three decimals and a power of ten that is a multiple of three: `900.000e3`. | Values quoted in units (k, M, G). |
| `Integer` | Whole numbers, rounded. | Counts, years, ranks. |
| `Percent` | The value times 100, with one decimal and a `%` sign: `70.0%`. | Proportions between 0 and 1. |""",
    examples=[
        dict(id="large", text="The formats on large values (300 000 to 900 000). `Auto` and `Plain` give the same labels on this data (`900000`); `Scientific` writes `9.00e+05` and `Engineering` writes `900.000e3`.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5};
const std::vector<double> y{300000, 450000, 520000, 710000, 900000};
const TickFormat formats[] = {TickFormat::Auto, TickFormat::Plain, TickFormat::Scientific, TickFormat::Engineering};
const char* names[] = {"Auto", "Plain", "Scientific", "Engineering"};

auto fig = studio("tick_format_large");
fig.mosaic(1, 4);
for (int i = 0; i < 4; ++i) {
  auto& pane = fig.pane2(names[i]);
  pane.trend(x, y);
  pane.y_tick_format(formats[i]);
}
fig.save_svg("figure.svg");"""),
        dict(id="small", text="The formats on small decimals (0.02 to 0.09). `Auto` writes `0.09`, `Fixed3` writes `0.090` and `Fixed6` writes `0.090000`: every label has the same number of decimals, which is what lets two panes be compared label by label.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5};
const std::vector<double> y{0.02, 0.045, 0.06, 0.075, 0.09};
const TickFormat formats[] = {TickFormat::Auto, TickFormat::Fixed3, TickFormat::Fixed6};
const char* names[] = {"Auto", "Fixed3", "Fixed6"};

auto fig = studio("tick_format_small");
fig.mosaic(1, 3);
for (int i = 0; i < 3; ++i) {
  auto& pane = fig.pane2(names[i]);
  pane.trend(x, y);
  pane.y_tick_format(formats[i]);
}
fig.save_svg("figure.svg");"""),
        dict(id="integer_percent", text="`Integer` and `Percent`. A series of years reads naturally as integers, and a series of proportions as percentages; the data in the second pane are fractions between 0 and 1, which `Percent` multiplies by 100 and writes with one decimal (`70.0%`). The x-axis of that pane is untouched.",
             code="""auto fig = studio("tick_format_integer_percent");
fig.mosaic(1, 2);
auto& years = fig.pane2("Integer");
years.trend(std::vector<double>{2018, 2019, 2020, 2021, 2022}, std::vector<double>{4, 7, 5, 9, 8});
years.x_tick_format(TickFormat::Integer);
auto& share = fig.pane2("Percent");
share.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{0.12, 0.25, 0.4, 0.55, 0.7});
share.y_tick_format(TickFormat::Percent);
fig.save_svg("figure.svg");"""),
        dict(id="per_axis", text="Each axis has its own format. The x-axis here is `Integer` and the y-axis `Scientific`; `x_axis().tick_format` and `y_axis().tick_format` report them separately.",
             code="""auto fig = studio("tick_format_per_axis");
auto& pane = fig.pane2("x Integer, y Scientific");
pane.trend(std::vector<double>{10, 20, 30, 40}, std::vector<double>{1.5e5, 4e5, 2.5e5, 9e5});
pane.x_tick_format(TickFormat::Integer);
pane.y_tick_format(TickFormat::Scientific);
std::cout << "x: " << static_cast<int>(pane.x_axis().tick_format) << ", y: " << static_cast<int>(pane.y_axis().tick_format) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="decimals", text="`Auto` with a decimals cap. `y_auto_tick_decimals(1)` limits the automatic form to one decimal, and `clear_y_auto_tick_decimals` removes the cap again. The cap changes the labels: the two figures are not equal.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5};
const std::vector<double> y{0.123, 0.241, 0.387, 0.452, 0.598};
std::string svg[2];
for (int i = 0; i < 2; ++i) {
  auto fig = studio("tick_format_decimals");
  auto& pane = fig.pane2(i == 0 ? "Auto" : "Auto, 1 decimal");
  pane.trend(x, y);
  if (i == 1) pane.y_auto_tick_decimals(1);
  svg[i] = fig.to_svg();
  if (i == 1) fig.save_svg("figure.svg");
}
std::cout << "capped == uncapped: " << (svg[0] == svg[1]) << "\\n";"""),
    ],
    notes="""- `TickFormat` does not rescale the data. `Percent` assumes values that are fractions; for data already in percent, use `Integer` or `Auto` and put the unit in the axis label (`x_label`, `y_label`).
- A format applies to the axis's major tick labels; labels given by hand with `x_ticks` or `x_categories` are text and are not reformatted.
- On a logarithmic axis the labels are the decade values, written in the chosen format.""",
    see=["[`Pane::x_tick_format`](../pane/x_tick_format.md), [`Pane::y_tick_format`](../pane/y_tick_format.md), [`Pane::z_tick_format`](../pane/z_tick_format.md): set the format.", "[`Pane::y_auto_tick_decimals`](../pane/y_auto_tick_decimals.md): the cap for `Auto`.", "[`ScalePolicy`](ScalePolicy.md), [`LimitMethod`](LimitMethod.md): the scale and the range."],
)
