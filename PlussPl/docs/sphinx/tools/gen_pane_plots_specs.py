#!/usr/bin/env python3
"""Method specs of the Pane trace-creating methods (planar kinds) into tools/method_specs/pane/.
Run, then `python3 tools/plotdocs.py --methods pane`. The volumetric kinds are in gen_pane_plots3_specs.py."""
import re, sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from gen_axis_specs import write as _write, TEXTS

def clean_signature(sig):
    out = []
    for line in sig.split("\n"):
        if line.startswith("template <"):
            continue
        line = re.sub(r"\b(?:Levels|Explode)&& (\w+)", r"std::vector<double> \1", line)
        line = re.sub(r"\b[A-Z]\w*&& (\w+)", r"Series \1", line)
        out.append(line)
    return "\n".join(out)


def write(slug, page):
    page['signature'] = clean_signature(page['signature'])
    _write(slug, page)


X5 = "std::vector<double>{1, 2, 3, 4, 5}"
Y5 = "std::vector<double>{2, 4, 3, 5, 6}"
Y5B = "std::vector<double>{3, 5, 4, 6, 7}"
GRID = 'auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3});'
BUMP = """const std::int64_t n = 25;
std::vector<double> axis(static_cast<std::size_t>(n)), field(static_cast<std::size_t>(n * n));
for (std::int64_t i = 0; i < n; ++i) axis[static_cast<std::size_t>(i)] = -2.0 + 4.0 * i / (n - 1);
for (std::int64_t i = 0; i < n; ++i) {
  for (std::int64_t j = 0; j < n; ++j) {
    const double x = axis[static_cast<std::size_t>(i)], y = axis[static_cast<std::size_t>(j)];
    field[static_cast<std::size_t>(i * n + j)] = std::exp(-(x * x + y * y)) + 0.5 * std::exp(-((x - 1.0) * (x - 1.0) + (y + 1.0) * (y + 1.0)));
  }
}
auto z = pluss::make_series(field, {n, n});"""


def head(name, kind="pane2"):
    return f'auto fig = studio("{name}");\nauto& pane = fig.{kind}("p");'


def ex(id, text, code):
    return dict(id=id, text=text, code=code)


def wrong_kind(method, call, planar=True):
    other = "pane3" if planar else "pane2"
    need = "planar" if planar else "volumetric"
    return ex("wrong_pane", f"`{method}` needs a {need} pane. On a {'volumetric' if planar else 'planar'} pane it throws at once, naming the kind it needs, and nothing is added.",
              f"""auto fig = studio("{method}_wrong_pane");
auto& pane = fig.{other}("p");
try {{
  {call};
}} catch (const pluss::Error& e) {{
  std::cout << e.message() << "\\n";
}}
std::cout << "traces: " << pane.trace_count() << "\\n";
fig.save_svg("figure.svg");""")


def errors(method, cases, planar=True, text=None):
    kind = "pane2" if planar else "pane3"
    body = "\n".join(f'report("{label}", [](Pane& pane) {{ {call}; }});' for label, call in cases)
    valid = ("pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});" if planar
             else "pane.path3(std::vector<double>{0, 1}, std::vector<double>{0, 1}, std::vector<double>{0, 1});")
    return ex("errors", text or "Inputs that cannot be drawn are refused when the trace is added, and the message names the method and the problem. The pane keeps the traces it had.",
              f"""auto report = [](const char* what, auto&& call) {{
  auto fig = studio("{method}_errors");
  auto& pane = fig.{kind}("p");
  try {{
    call(pane);
    std::cout << what << ": accepted\\n";
  }} catch (const pluss::Error& e) {{
    std::cout << what << ": " << e.message() << "\\n";
  }}
}};
{body}
auto fig = studio("{method}_errors_out");
auto& pane = fig.{kind}("p");
{valid}
fig.save_svg("figure.svg");""")


def trace_notes(extra=""):
    return ("- Returns the new [`Trace&`](../trace/label.md); the reference stays valid for the life of the pane, and the style calls of the trace ([`label`](../trace/label.md), [`stroke`](../trace/stroke.md), [`width`](../trace/width.md), ...) chain off it.\n"
            "- The data is copied into the trace when it is added, so changing the vector afterwards does not change the figure.\n" + extra)


# ---------------------------------------------------------------- trend
def trend():
    ex_ = [
        ex("basic", "A line through five points. Points are joined in the order they are given.",
           f"""{head('trend_basic')}
Trace& line = pane.trend({X5}, {Y5});
std::cout << "traces: " << pane.trace_count() << "\\n";
fig.save_svg("figure.svg");"""),
        ex("order", "The order is the order of the data: `trend` does not sort by x. The same five points in a different order make a different path.",
           f"""auto fig = studio("trend_order");
fig.mosaic(1, 2);
auto& sorted = fig.pane2("sorted by x");
sorted.trend(std::vector<double>{{1, 2, 3, 4, 5}}, std::vector<double>{{1, 3, 2, 5, 4}});
auto& shuffled = fig.pane2("as given");
shuffled.trend(std::vector<double>{{3, 1, 5, 2, 4}}, std::vector<double>{{2, 1, 4, 3, 5}});
fig.save_svg("figure.svg");"""),
        ex("two", "Several traces on one pane, each added by its own call, take the accent colours of the theme in turn.",
           f"""{head('trend_two')}
pane.trend({X5}, {Y5}).label("first");
pane.trend({X5}, {Y5B}).label("second");
pane.legend(KeyPlace::NorthWest);
std::cout << "traces: " << pane.trace_count() << "\\n";
fig.save_svg("figure.svg");"""),
        ex("broadcast", "A single value is repeated to the length of the other series, so a flat line is `trend(x, 3.0)` written as a one-element series.",
           f"""{head('trend_broadcast')}
pane.trend({X5}, pluss::series::linspace(3.0, 3.0, 1, pluss::ScalarType::Float64));
pane.trend({X5}, {Y5});
fig.save_svg("figure.svg");"""),
        ex("types", "Anything that converts to a `Series` is accepted: `std::vector` of any arithmetic type, an initializer list, or a `Series`. The three calls draw the same line.",
           f"""auto build = [&](int how) {{
  auto fig = studio("trend_types");
  auto& pane = fig.pane2("p");
  if (how == 0) pane.trend(std::vector<double>{{1, 2, 3}}, std::vector<double>{{2, 4, 3}});
  if (how == 1) pane.trend(std::vector<int>{{1, 2, 3}}, std::vector<float>{{2, 4, 3}});
  if (how == 2) pane.trend(pluss::series::linspace(1.0, 3.0, 3, pluss::ScalarType::Float64), std::vector<double>{{2, 4, 3}});
  return fig.to_svg();
}};
std::cout << "vector<int>/<float> == vector<double>: " << (build(1) == build(0)) << ", linspace Series == vector: " << (build(2) == build(0)) << "\\n";
{head('trend_types_out')}
pane.trend(std::vector<double>{{1, 2, 3}}, std::vector<double>{{2, 4, 3}});
fig.save_svg("figure.svg");"""),
        ex("single", "One point is a valid trace, with nothing to join: it adds a trace that draws no line, and the pane's range still comes from it.",
           """auto fig = studio("trend_single");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{2.0}, std::vector<double>{3.0});
std::cout << "traces: " << pane.trace_count() << "\\n";
fig.save_svg("figure.svg");"""),
        errors("trend", [("lengths differ", "pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2})"),
                         ("empty", "pane.trend(std::vector<double>{}, std::vector<double>{})")]),
        wrong_kind("trend", "pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1})"),
    ]
    write("trend", dict(
        slug="trend", title="Pane::trend",
        signature="template <class X, class Y>\nTrace& Pane::trend(X&& x, Y&& y)",
        intro="""Adds a **line**: the points `(x[i], y[i])` joined in the order given. It is the basic trace of a plot of one quantity against another and the one every line chart of [`flash`](../../flash_api_reference) is built on.

`x` and `y` are anything that converts to a `Series`. They must have the same length, or one of them a single value, which is repeated. Empty series are refused. The line is not sorted: put the points in the order they should be joined. Series with `NaN` are accepted and drawn as far as the other values go.

The pane needs to be planar ([`pane2`](../studio/pane2.md)); a volumetric pane takes [`path3`](path3.md) instead.""",
        parameters="""**x**, **y** (`Series`-convertible)
    The coordinates of the points. Equal lengths, or one of length 1.""",
        returns="""`Trace&`
    The new trace, for styling.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::path3`](path3.md): the line in three dimensions.", "[`Pane::cloud`](cloud.md): the points without the line.", "[`Pane::ribbon`](ribbon.md): the region under a line."]))


# ---------------------------------------------------------------- cloud
def cloud():
    ex_ = [
        ex("plain", "Points at `(x[i], y[i])`, all of one size and colour.",
           f"""{head('cloud_plain')}
pane.cloud({X5}, {Y5});
fig.save_svg("figure.svg");"""),
        ex("size", "A third series gives each point its own size: a bubble chart. The size is an area: a point of size 100 has the area of four points of size 25, and so twice their radius. Here the sizes grow from 20 to 100.",
           f"""{head('cloud_size')}
pane.cloud({X5}, {Y5}, std::vector<double>{{20, 40, 60, 80, 100}});
fig.save_svg("figure.svg");"""),
        ex("size_scalar", "A single size is repeated for every point, so a series of equal markers can be larger or smaller than the default. Sizes 25 and 100 are two and one scale.",
           f"""auto build = [&](double size) {{
  auto fig = studio("cloud_size_scalar");
  auto& pane = fig.pane2("p");
  pane.cloud({X5}, {Y5}, std::vector<double>{{size}});
  return fig.to_svg();
}};
std::cout << "size 25 != size 100: " << (build(25.0) != build(100.0)) << "\\n";
{head('cloud_size_scalar_out')}
pane.cloud({X5}, {Y5}, std::vector<double>{{100.0}});
fig.save_svg("figure.svg");"""),
        ex("color", "A fourth series colours the points through the palette, from its lowest value to its highest. The colour guide, switched on with [`scale_guide`](scale_guide.md), shows which value each colour stands for.",
           f"""{head('cloud_color')}
pane.cloud({X5}, {Y5}, std::vector<double>{{3, 3, 3, 3, 3}}, std::vector<double>{{0.1, 0.3, 0.5, 0.7, 0.9}});
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("size_and_color", "Size and colour together: two more quantities on the same plot.",
           f"""{head('cloud_both')}
pane.cloud({X5}, {Y5}, std::vector<double>{{5, 4, 3, 2, 1}}, std::vector<double>{{10, 20, 30, 40, 50}});
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("color_scalar", "A single colour value is repeated: every point takes the same place in the palette.",
           f"""{head('cloud_color_scalar')}
pane.cloud({X5}, {Y5}, std::vector<double>{{3}}, std::vector<double>{{0.8}});
fig.save_svg("figure.svg");"""),
        errors("cloud", [("x and y differ", "pane.cloud(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2})"),
                         ("size differs", "pane.cloud(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 2})"),
                         ("color differs", "pane.cloud(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 1, 1}, std::vector<double>{1, 2})")]),
        wrong_kind("cloud", "pane.cloud(std::vector<double>{0, 1}, std::vector<double>{0, 1})"),
    ]
    write("cloud", dict(
        slug="cloud", title="Pane::cloud",
        signature="template <class X, class Y>\nTrace& Pane::cloud(X&& x, Y&& y)\ntemplate <class X, class Y, class Size>\nTrace& Pane::cloud(X&& x, Y&& y, Size&& size)\ntemplate <class X, class Y, class Size, class Color>\nTrace& Pane::cloud(X&& x, Y&& y, Size&& size, Color&& color)",
        intro="""Adds a **cloud**: a marker at each point `(x[i], y[i])`, with nothing joining them. It is a scatter plot, and with the optional series it carries two more quantities in the size and the colour of the markers.

| Call | Adds |
|---|---|
| `cloud(x, y)` | Markers of one size and one colour. |
| `cloud(x, y, size)` | A size for each marker (or one size for all). |
| `cloud(x, y, size, color)` | A colour for each marker too (or one for all), taken from the palette over the range of `color`. |

There is no call with a colour and no size: pass a single size to keep one marker size for all. The range the colours are mapped over is the data's own, unless [`color_limits`](color_limits.md) or [`color_limit_method`](color_limit_method.md) says otherwise; the palette is the trace's ([`Trace::palette`](../trace/palette.md)).  
The size is the area of the marker, so the radius grows as its square root; the radius is `0.31 * sqrt(size)` points. A marker is never smaller than a visible dot, about one point in radius. When the sizes of a trace are small, their scale is raised together so that the smallest positive size reaches that dot and the sizes still differ in the same ratio; a single size (one value for all) is drawn at least that big. All series have the same length, or length 1.""",
        parameters="""**x**, **y** (`Series`-convertible)
    The positions.

**size** (`Series`-convertible, optional)
    The size of each marker, or one size for all.

**color** (`Series`-convertible, optional)
    The value that colours each marker, or one value for all.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::cloud3`](cloud3.md): markers in three dimensions.", "[`Pane::scale_guide`](scale_guide.md): the key to the colours.", "[`Pane::color_limits`](color_limits.md): the range of the colours.", "[`Pane::trend`](trend.md): the line through points."]))


# ---------------------------------------------------------------- columns / columns_h
def columns(horizontal):
    name = "columns_h" if horizontal else "columns"
    pos, val = ("y", "x") if horizontal else ("x", "y")
    one = f"pane.{name}(std::vector<double>{{3, 5, 2, 6}})"
    two = f"pane.{name}(std::vector<double>{{10, 20, 30, 40}}, std::vector<double>{{3, 5, 2, 6}})" if not horizontal else f"pane.{name}(std::vector<double>{{3, 5, 2, 6}}, std::vector<double>{{10, 20, 30, 40}})"
    neg = f"std::vector<double>{{3, -2, 4, -1}}"
    ex_ = [
        ex("one", f"One series. The values are the {'lengths' if horizontal else 'heights'} of the bars, and the {pos} positions are the indices 1, 2, 3, 4.",
           f"""{head(name + '_one')}
{one};
fig.save_svg("figure.svg");"""),
        ex("two", f"Two series: the first is the {'bar lengths' if horizontal else 'positions'}, the second the {'positions' if horizontal else 'heights'}. The bars stand at the positions given, 10, 20, 30 and 40, and are 8 wide, 0.8 of the gap of 10 between neighbours.",
           f"""{head(name + '_two')}
{two};
fig.save_svg("figure.svg");"""),
        ex("negative", "Values below zero run the other way: bars are drawn from zero to the value, so negative values go down (or left).",
           f"""{head(name + '_negative')}
pane.{name}({neg});
fig.save_svg("figure.svg");"""),
        ex("broadcast", "A single value is repeated for every position: equal bars at the positions given.",
           f"""{head(name + '_broadcast')}
pane.{name}({'std::vector<double>{4}, std::vector<double>{1, 2, 3}' if horizontal else 'std::vector<double>{1, 2, 3}, std::vector<double>{4}'});
fig.save_svg("figure.svg");"""),
        ex("two_series", "Two traces on one pane are drawn over one another at the same positions, the later in front: useful for a total and a part of it.",
           f"""{head(name + '_two_series')}
pane.{name}(std::vector<double>{{5, 6, 4, 7}}).label("total");
pane.{name}(std::vector<double>{{3, 2, 3, 4}}).label("part");
pane.legend(KeyPlace::NorthEast);
fig.save_svg("figure.svg");"""),
        ex("spacing", "The bars adapt to the spacing of their positions: they are 0.8 of the smallest gap between two positions wide. Positions 1, 2, 5 and 6 (gap 1) give bars 0.8 wide; positions 10, 20, 50 and 60 (gap 10) give bars 8 wide, so the same picture is drawn at any scale.",
           f"""auto fig = studio("%s_spacing");
fig.mosaic(1, 2);
auto& near = fig.pane2("positions 1, 2, 5, 6");
near.%s(%s);
auto& far = fig.pane2("positions 10, 20, 50, 60");
far.%s(%s);
fig.save_svg("figure.svg");""" % (name, name, "std::vector<double>{1, 2, 5, 6}, std::vector<double>{3, 5, 2, 6}" if not horizontal else "std::vector<double>{3, 5, 2, 6}, std::vector<double>{1, 2, 5, 6}", name, "std::vector<double>{10, 20, 50, 60}, std::vector<double>{3, 5, 2, 6}" if not horizontal else "std::vector<double>{3, 5, 2, 6}, std::vector<double>{10, 20, 50, 60}")),
        ex("outline", "The outline of the bars is a line of the trace, set with [`Trace::width`](../trace/width.md) and [`Trace::stroke`](../trace/stroke.md); the width of the bars themselves follows the positions.",
           f"""{head(name + '_outline')}
pane.%s(std::vector<double>{{3, 5, 2, 6}}).width(3.0);
fig.save_svg("figure.svg");""" % name),
        errors(name, [("lengths differ", f"pane.{name}(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2}})")]),
        wrong_kind(name, f"pane.{name}(std::vector<double>{{1, 2}})"),
    ]
    write(name, dict(
        slug=name, title=f"Pane::{name}",
        signature=(f"template <class X>\nTrace& Pane::{name}(X&& x)\ntemplate <class X, class Y>\nTrace& Pane::{name}(X&& x, Y&& y)" if horizontal else f"template <class Y>\nTrace& Pane::{name}(Y&& y)\ntemplate <class X, class Y>\nTrace& Pane::{name}(X&& x, Y&& y)"),
        intro=f"""Adds a **{'horizontal ' if horizontal else ''}bar chart**: one bar for each value, standing on the {'y' if horizontal else 'x'} axis and running {'to the right' if horizontal else 'up'} (or {'left' if horizontal else 'down'} for a negative value) from zero to the value. {'It is the horizontal counterpart of [`columns`](columns.md): the two make the same kind of trace, drawn along different axes, and everything said of one holds for the other with the axes exchanged.' if horizontal else 'The horizontal form is [`columns_h`](columns_h.md).'}

| Call | Meaning |
|---|---|
| `{name}({'x' if horizontal else 'y'})` | The {'lengths' if horizontal else 'heights'} of the bars; they stand at the positions 1, 2, 3, ... |
| `{name}(x, y)` | {'The lengths along x and the positions along y.' if horizontal else 'The positions along x and the heights along y.'} |

The two series have the same length, or one of them has length 1. The bars are 0.8 of the smallest gap between two positions wide (0.8 for the positions 1, 2, 3, ...); their outline is set by [`Trace::width`](../trace/width.md). Bars of several traces on one pane are drawn over one another; to place them side by side, give each its own positions, offset by less than a bar. For counts of raw data use [`bucket`](bucket.md), which does the counting.""",
        parameters=f"""**{'x' if horizontal else 'y'}** (`Series`-convertible)
    The bar {'lengths' if horizontal else 'heights'} (single-argument call) or, in the two-argument call, the {'lengths' if horizontal else 'positions'}.

**{'y' if horizontal else 'x'}** (`Series`-convertible, two-argument call)
    The {'positions' if horizontal else 'heights'}.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=[f"[`Pane::{'columns' if horizontal else 'columns_h'}`]({'columns' if horizontal else 'columns_h'}.md): the {'vertical' if horizontal else 'horizontal'} form.", "[`Pane::bucket`](bucket.md): bars from raw data, counted.", "[`Pane::x_categories`](x_categories.md): names under the bars.", "[`Trace::width`](../trace/width.md): the outline of the bars."]))


# ---------------------------------------------------------------- bucket
def bucket():
    data = "std::vector<double>{1, 2, 2, 3, 3, 3, 4, 4, 5, 9}"
    ex_ = [
        ex("default", "Ten values counted into 12 bins by default. The bins span the data's own range, 1 to 9.",
           f"""{head('bucket_default')}
pane.bucket({data});
fig.save_svg("figure.svg");"""),
        ex("bins", "The number of bins is the second argument. Five bins gives wider bars with larger counts, and the tallest bar is read from the axis.",
           f"""auto build = [&](int bins) {{
  auto fig = studio("bucket_bins");
  auto& pane = fig.pane2("p");
  pane.x_tick_labels(false);
  pane.bucket({data}, bins);
  return fig.to_svg();
}};
{TEXTS}
std::cout << "3 bins:  " << texts(build(3)) << "\\n";
std::cout << "12 bins: " << texts(build(12)) << "\\n";
{head('bucket_bins_out')}
pane.bucket({data}, 5);
fig.save_svg("figure.svg");"""),
        ex("range", "An explicit range replaces the data's own: the bins divide the range given. Here 8 bins span 0 to 10, a little wider than the data, and the bars stand in their places on that scale.",
           f"""{head('bucket_range')}
pane.bucket({data}, 8, Range1d{{0.0, 10.0}});
fig.save_svg("figure.svg");"""),
        ex("two_traces", "To compare two samples on one scale, give both the same range and the same number of bins.",
           f"""{head('bucket_two')}
pane.bucket(std::vector<double>{{1, 2, 2, 3, 3, 3, 4, 4, 5, 9}}, 9, Range1d{{0.0, 10.0}}).label("sample a");
pane.bucket(std::vector<double>{{5, 6, 6, 7, 7, 7, 8, 8, 9, 2}}, 9, Range1d{{0.0, 10.0}}).label("sample b");
pane.legend(KeyPlace::NorthWest);
fig.save_svg("figure.svg");"""),
        ex("integers", "Integer data is accepted as it is.",
           f"""{head('bucket_ints')}
pane.bucket(std::vector<int>{{1, 2, 2, 3, 3, 3, 4, 4, 5, 9}}, 6);
fig.save_svg("figure.svg");"""),
        errors("bucket", [("no bins", "pane.bucket(std::vector<double>{1, 2, 3}, 0)"), ("negative bins", "pane.bucket(std::vector<double>{1, 2, 3}, -3)"),
                          ("range reversed", "pane.bucket(std::vector<double>{1, 2, 3}, 4, Range1d{3.0, 1.0})"), ("no values", "pane.bucket(std::vector<double>{}, 4)")]),
        wrong_kind("bucket", "pane.bucket(std::vector<double>{1, 2, 3})"),
    ]
    write("bucket", dict(
        slug="bucket", title="Pane::bucket",
        signature="template <class Values>\nTrace& Pane::bucket(Values&& values, int bins = 12)\ntemplate <class Values>\nTrace& Pane::bucket(Values&& values, int bins, Range1d range)",
        intro="""Adds a **histogram**: the raw `values` are counted into equal-width bins and a bar is drawn for the count of each. Where [`columns`](columns.md) draws bars of heights you give, `bucket` finds the heights itself.

| Call | Bins |
|---|---|
| `bucket(values, bins = 12)` | `bins` bins of equal width over the range of the data. |
| `bucket(values, bins, range)` | `bins` bins of equal width over `range`, which may be wider or narrower than the data. |

`bins` must be positive, `range` finite with `low` below `high`, and there must be at least one value; otherwise the call throws. The y axis shows counts. Use an explicit `range` and `bins` whenever two histograms are to be compared, so that their bars line up.""",
        parameters="""**values** (`Series`-convertible)
    The data to count. Not empty.

**bins** (`int`, default 12)
    The number of bins. Positive.

**range** (`Range1d`, optional)
    The interval to divide into bins. `low` below `high`, both finite.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::columns`](columns.md): bars of heights you supply.", "[`Pane::violin`](violin.md), [`Pane::quartet`](quartet.md): other summaries of a sample."]))


# ---------------------------------------------------------------- quartet
def quartet():
    ex_ = [
        ex("one", "One summary over all the values: the box spans the middle half of the data, the line in it is the median, and the whiskers reach out to the extremes that are not outliers.",
           f"""{head('quartet_one')}
pane.quartet(std::vector<double>{{1, 2, 3, 4, 5, 6, 7, 8, 9, 30}});
fig.save_svg("figure.svg");"""),
        ex("groups", "A group series splits the values into groups, one box for each distinct group value, in the order the groups first appear.",
           f"""{head('quartet_groups')}
pane.quartet(std::vector<double>{{1, 1, 1, 1, 1, 2, 2, 2, 2, 2}}, std::vector<double>{{3, 4, 5, 6, 7, 6, 7, 8, 9, 10}});
fig.save_svg("figure.svg");"""),
        ex("three", "Three groups, to compare the spread of three samples at a glance.",
           f"""{head('quartet_three')}
std::vector<double> group, value;
for (int g = 0; g < 3; ++g) {{
  for (int i = 0; i < 8; ++i) {{
    group.push_back(g);
    value.push_back(10.0 * g + 3.0 * std::sin(1.7 * i + g) + i);
  }}
}}
pane.quartet(group, value);
fig.save_svg("figure.svg");"""),
        ex("outlier", "A value far from the rest is drawn on its own beyond the whisker rather than stretching the box: the single 30 above stands apart from the data that runs to 9.",
           f"""auto fig = studio("quartet_outlier");
fig.mosaic(1, 2);
auto& with = fig.pane2("with an outlier");
with.quartet(std::vector<double>{{1, 2, 3, 4, 5, 6, 7, 8, 9, 30}});
auto& without = fig.pane2("without");
without.quartet(std::vector<double>{{1, 2, 3, 4, 5, 6, 7, 8, 9}});
fig.save_svg("figure.svg");"""),
        errors("quartet", [("group and values differ", "pane.quartet(std::vector<double>{1, 2}, std::vector<double>{1, 2, 3})"), ("no values", "pane.quartet(std::vector<double>{})")]),
        wrong_kind("quartet", "pane.quartet(std::vector<double>{1, 2, 3})"),
    ]
    write("quartet", dict(
        slug="quartet", title="Pane::quartet",
        signature="template <class Values>\nTrace& Pane::quartet(Values&& values)\ntemplate <class Group, class Values>\nTrace& Pane::quartet(Group&& group, Values&& values)",
        intro="""Adds a **box-and-whisker** summary of a sample: a box from the first to the third quartile with a line at the median, whiskers to the most extreme values that are not outliers, and the outliers as separate points. It shows the centre, the spread and the skew of a sample in a small space, and is the usual way to compare several samples.

| Call | Draws |
|---|---|
| `quartet(values)` | One box for all the values. |
| `quartet(group, values)` | One box for each distinct value of `group`, over the values in that group. |

`group` and `values` have the same length. There must be at least one value. Several boxes in one trace share one colour; to give samples different colours add them as separate traces.""",
        parameters="""**values** (`Series`-convertible)
    The sample. Not empty.

**group** (`Series`-convertible, optional)
    The group of each value; same length as `values`.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::violin`](violin.md): the shape of the distribution.", "[`Pane::bucket`](bucket.md): the histogram."]))


# ---------------------------------------------------------------- violin
def violin():
    s = "std::vector<double>{1.0, 1.4, 1.7, 2.0, 2.1, 2.4, 2.6, 3.0, 3.3, 3.5, 4.1, 5.0}"
    g = "std::vector<double>{1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}"
    s2 = "std::vector<double>{1.0, 1.4, 1.7, 2.0, 2.1, 2.4, 3.6, 4.0, 4.3, 4.5, 5.1, 6.0}"
    ex_ = [
        ex("one", "One violin for all the values: the outline is a smoothed estimate of how the values are distributed, wide where they are dense.",
           f"""{head('violin_one')}
pane.violin({s});
fig.save_svg("figure.svg");"""),
        ex("options", "With a `ViolinOptions` the shape is chosen: here the violin is narrower, smoothed more, and drawn as one half only. Each field is described on the [`ViolinOptions`](../types/ViolinOptions.md) page.",
           f"""{head('violin_options')}
ViolinOptions options;
options.width = 0.5;
options.bw_adjust = 1.5;
options.side = ViolinSide::Right;
pane.violin({s}, options);
fig.save_svg("figure.svg");"""),
        ex("groups", "A group series gives one violin for each distinct group value.",
           f"""{head('violin_groups')}
pane.violin({g}, {s2});
fig.save_svg("figure.svg");"""),
        ex("groups_options", "Groups and options together.",
           f"""{head('violin_groups_options')}
ViolinOptions options;
options.inner = ViolinInner::Quartile;
options.scale = ViolinScale::Width;
pane.violin({g}, {s2}, options);
fig.save_svg("figure.svg");"""),
        ex("defaults", "The calls without options use `ViolinOptions{}`: the two figures are the same.",
           f"""auto build = [&](bool explicit_options) {{
  auto fig = studio("violin_defaults");
  auto& pane = fig.pane2("p");
  if (explicit_options) pane.violin({s}, ViolinOptions{{}});
  else pane.violin({s});
  return fig.to_svg();
}};
std::cout << "same figure: " << (build(true) == build(false)) << "\\n";
{head('violin_defaults_out')}
pane.violin({s});
fig.save_svg("figure.svg");"""),
        ex("one_value", "A single value has no distribution to estimate. It is accepted and drawn as a thin violin at that value.",
           """auto fig = studio("violin_one_value");
auto& pane = fig.pane2("p");
pane.violin(std::vector<double>{3.0});
std::cout << "traces: " << pane.trace_count() << "\\n";
fig.save_svg("figure.svg");"""),
        errors("violin", [("width zero", "ViolinOptions o; o.width = 0.0; pane.violin(std::vector<double>{1, 2, 3, 4}, o)"),
                          ("smoothing negative", "ViolinOptions o; o.bw_adjust = -1.0; pane.violin(std::vector<double>{1, 2, 3, 4}, o)"),
                          ("cut negative", "ViolinOptions o; o.cut = -1.0; pane.violin(std::vector<double>{1, 2, 3, 4}, o)"),
                          ("group and values differ", "pane.violin(std::vector<double>{1, 2}, std::vector<double>{1, 2, 3})")]),
        wrong_kind("violin", "pane.violin(std::vector<double>{1, 2, 3})"),
    ]
    write("violin", dict(
        slug="violin", title="Pane::violin",
        signature="template <class Values>\nTrace& Pane::violin(Values&& values)\ntemplate <class Values>\nTrace& Pane::violin(Values&& values, const ViolinOptions& options)\ntemplate <class Group, class Values>\nTrace& Pane::violin(Group&& group, Values&& values)\ntemplate <class Group, class Values>\nTrace& Pane::violin(Group&& group, Values&& values, const ViolinOptions& options)",
        intro="""Adds a **violin plot**: a smoothed estimate of the distribution of a sample, drawn as a mirrored outline that is wide where the values are dense. Unlike a [box](quartet.md) it shows bumps and gaps in the distribution, and unlike a [histogram](bucket.md) it has no bins to choose.

The four calls are the two choices, grouped or not and with options or not:

| Call | Draws |
|---|---|
| `violin(values)` | One violin, default options. |
| `violin(values, options)` | One violin, shaped by `options`. |
| `violin(group, values)` | One violin per distinct group, default options. |
| `violin(group, values, options)` | One per group, shaped by `options`. |

[`ViolinOptions`](../types/ViolinOptions.md) holds the width of the violins, how much the estimate is smoothed, how far the outline extends past the data, whether the groups are scaled by area, by count or to a common width, what is drawn inside, and which side is drawn. The options are checked when the trace is added: `width` and `bw_adjust` must be positive and finite, `cut` non-negative and finite.""",
        parameters="""**values** (`Series`-convertible)
    The sample.

**group** (`Series`-convertible, optional)
    The group of each value; same length as `values`.

**options** (`ViolinOptions`, optional)
    The shape of the violins.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`ViolinOptions`](../types/ViolinOptions.md): every field, with a figure for each.", "[`Pane::quartet`](quartet.md), [`Pane::bucket`](bucket.md): other summaries of a sample."]))


# ---------------------------------------------------------------- radial
def radial():
    PI = "3.141592653589793"
    ex_ = [
        ex("basic", "Angle and radius for each point, the angle in radians counted anticlockwise from the right. Five points at equal angles of a fifth of a turn, with their radii.",
           f"""auto fig = studio("radial_basic");
auto& pane = fig.pane2("p");
std::vector<double> theta, radius{{3, 5, 2, 4, 5}};
for (int i = 0; i < 5; ++i) theta.push_back(2.0 * {PI} * i / 5.0);
pane.radial(theta, radius);
fig.save_svg("figure.svg");"""),
        ex("closed", "The points are joined in the order given and the last is not joined to the first. To close a shape, repeat the first point at the end: the second figure is closed.",
           f"""auto fig = studio("radial_closed");
fig.mosaic(1, 2);
std::vector<double> theta, radius{{3, 5, 2, 4, 5}};
for (int i = 0; i < 5; ++i) theta.push_back(2.0 * {PI} * i / 5.0);
auto& open = fig.pane2("open");
open.radial(theta, radius);
theta.push_back(theta.front());
radius.push_back(radius.front());
auto& closed = fig.pane2("closed");
closed.radial(theta, radius);
fig.save_svg("figure.svg");"""),
        ex("rose", "Many points make a smooth curve: a three-petal rose, `r = 1 + cos(3 theta)`, sampled every 5 degrees (theta in radians).",
           f"""auto fig = studio("radial_rose");
auto& pane = fig.pane2("p");
std::vector<double> theta, radius;
for (int degrees = 0; degrees <= 360; degrees += 5) {{
  const double t = degrees * {PI} / 180.0;
  theta.push_back(t);
  radius.push_back(1.0 + std::cos(3.0 * t));
}}
pane.radial(theta, radius);
fig.save_svg("figure.svg");"""),
        ex("two", "Two traces on one polar pane, to compare two profiles on the same scale: six directions each, both closed.",
           f"""auto fig = studio("radial_two");
auto& pane = fig.pane2("p");
std::vector<double> theta;
for (int i = 0; i <= 6; ++i) theta.push_back(2.0 * {PI} * i / 6.0);
pane.radial(theta, std::vector<double>{{4, 5, 3, 4, 2, 5, 4}}).label("this year");
pane.radial(theta, std::vector<double>{{2, 3, 4, 2, 4, 3, 2}}).label("last year");
pane.legend(KeyPlace::NorthEast);
fig.save_svg("figure.svg");"""),
        ex("broadcast", "A single radius is repeated for every angle: points on a circle of that radius, here 33 points around it.",
           f"""auto fig = studio("radial_circle");
auto& pane = fig.pane2("p");
std::vector<double> theta;
for (int i = 0; i <= 32; ++i) theta.push_back(2.0 * {PI} * i / 32.0);
pane.radial(theta, std::vector<double>{{4}});
fig.save_svg("figure.svg");"""),
        errors("radial", [("lengths differ", "pane.radial(std::vector<double>{0, 1.5, 3}, std::vector<double>{1, 2})")]),
        wrong_kind("radial", "pane.radial(std::vector<double>{0, 1}, std::vector<double>{1, 2})"),
    ]
    write("radial", dict(
        slug="radial", title="Pane::radial",
        signature="template <class Theta, class Radius>\nTrace& Pane::radial(Theta&& theta, Radius&& radius)",
        intro="""Adds a **polar trace**: points given by an angle and a distance from the centre, joined in the order given. It is the trace of radar (spider) charts and of curves written in polar form. `theta` is in **radians**, counted anticlockwise from the direction of the positive x axis (to the right); `radius` is the distance from the centre, in the units of the pane. The trace is an open line: repeat the first point at the end to close a shape. A pane that holds only radial traces is drawn as a round plot with rings and spokes instead of x and y axes.

`theta` and `radius` have the same length, or one of them has length 1.""",
        parameters="""**theta** (`Series`-convertible)
    The angles, in radians.

**radius** (`Series`-convertible)
    The distances from the centre.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::trend`](trend.md): points on x and y axes."]))


# ---------------------------------------------------------------- ribbon / spike
def ribbon_spike(slug):
    ribbon = slug == "ribbon"
    ex_ = [
        ex("y", f"Values only: the positions are the indices 1, 2, 3, ... {'The region between the values and the base (0) is filled.' if ribbon else 'A stem rises from the base (0) to each value.'}",
           f"""{head(slug + '_y')}
pane.{slug}(std::vector<double>{{2, 4, 3, 5, 4, 6}});
fig.save_svg("figure.svg");"""),
        ex("xy", "Positions and values. The positions need not be evenly spaced.",
           f"""{head(slug + '_xy')}
pane.{slug}(std::vector<double>{{0, 1, 3, 4, 8, 9}}, std::vector<double>{{2, 4, 3, 5, 4, 6}});
fig.save_svg("figure.svg");"""),
        ex("base", f"The base moves the baseline. With base 3 the {'band' if ribbon else 'stems'} {'is' if ribbon else 'are'} drawn between 3 and each value, so values below 3 {'fill upward from the line to the data' if ribbon else 'give stems that hang down'}.",
           f"""{head(slug + '_base')}
pane.{slug}(std::vector<double>{{2, 4, 3, 5, 4, 6}}, 3.0);
pane.rule_y(3.0, "base");
fig.save_svg("figure.svg");"""),
        ex("base_xy", "The base with positions: the second overload takes it after the data.",
           f"""{head(slug + '_base_xy')}
pane.{slug}(std::vector<double>{{0, 1, 3, 4, 8, 9}}, std::vector<double>{{2, 4, 3, 5, 4, 6}}, 1.0);
fig.save_svg("figure.svg");"""),
        ex("negative", "Negative values lie below the base, which is zero by default.",
           f"""{head(slug + '_negative')}
pane.{slug}(std::vector<double>{{2, -1, 3, -2, 4}});
fig.save_svg("figure.svg");"""),
    ]
    if ribbon:
        ex_ += [ex("matrix", "A rank-2 `y` makes one band for each column, **stacked**: the rows are the positions, and each band is drawn on top of the ones before it, so its upper edge is the running total. Here two columns give two bands; the upper edge of the second is the sum of the two.",
                   """auto fig = studio("ribbon_matrix");
auto& pane = fig.pane2("p");
auto y = pluss::make_series(std::vector<double>{2, 1, 4, 2, 3, 3, 5, 2}, {4, 2});
pane.ribbon(y);
std::cout << "traces: " << pane.trace_count() << "\\n";
fig.save_svg("figure.svg");""")]
    ex_ += [errors(slug, [("x and y differ", f"pane.{slug}(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2}})")]), wrong_kind(slug, f"pane.{slug}(std::vector<double>{{1, 2}})")]
    write(slug, dict(
        slug=slug, title=f"Pane::{slug}",
        signature=f"template <class Y>\nTrace& Pane::{slug}(Y&& y, double base = 0.0)\ntemplate <class X, class Y>\nTrace& Pane::{slug}(X&& x, Y&& y, double base = 0.0)",
        intro=(f"""Adds a **filled band**: the region between a baseline and a curve through the values, the trace of an area chart. The baseline is `base`, 0 by default, so a positive series is filled down to the axis; a different `base` is a different floor.

| Call | Meaning |
|---|---|
| `ribbon(y, base = 0)` | Values at the positions 1, 2, 3, ... |
| `ribbon(x, y, base = 0)` | Values at the positions `x`. |

A rank-2 `y` is read as one series per column, with a band for each, stacked one on top of the next. `x` and `y` have as many rows as each other. For the same data as vertical stems rather than a filled region use [`spike`](spike.md); for the line alone, [`trend`](trend.md)."""
               if ribbon else f"""Adds **stems**: one vertical line for each value, from a baseline to the value, with nothing joining them. It is the trace of a discrete signal, a stem plot or a lollipop chart. The baseline is `base`, 0 by default.

| Call | Meaning |
|---|---|
| `spike(y, base = 0)` | Values at the positions 1, 2, 3, ... |
| `spike(x, y, base = 0)` | Values at the positions `x`. |

`x` and `y` have the same length, or one has length 1. For the same data as a filled region use [`ribbon`](ribbon.md)."""),
        parameters=f"""**{'x' if True else ''}** (`Series`-convertible, second call)
    The positions.

**y** (`Series`-convertible)
    The values.

**base** (`double`, default 0)
    The baseline the {'band' if ribbon else 'stems'} start{'s' if not ribbon else ''} from.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=[f"[`Pane::{'spike' if ribbon else 'ribbon'}`]({'spike' if ribbon else 'ribbon'}.md): the {'stems' if ribbon else 'filled band'}.", "[`Pane::trend`](trend.md): the line.", "[`Pane::columns`](columns.md): bars."]))


# ---------------------------------------------------------------- wedge
def wedge():
    ex_ = [
        ex("values", "Values only: each wedge's angle is its share of the total, starting from the top and going clockwise. Each wedge is annotated with its share as a percentage.",
           f"""{head('wedge_values')}
pane.wedge(std::vector<double>{{4, 3, 2, 1}});
fig.save_svg("figure.svg");"""),
        ex("labels", "Labels name the wedges, in the order of the values.",
           f"""{head('wedge_labels')}
pane.wedge(std::vector<double>{{4, 3, 2, 1}}, {{"rent", "food", "travel", "other"}});
fig.save_svg("figure.svg");"""),
        ex("explode", "A third series pulls wedges out of the pie: 0 keeps a wedge in place, larger values move it further away.",
           f"""{head('wedge_explode')}
pane.wedge(std::vector<double>{{4, 3, 2, 1}}, std::vector<double>{{0, 0, 0.2, 0}}, {{"rent", "food", "travel", "other"}});
fig.save_svg("figure.svg");"""),
        ex("shares", "Only the shares matter: values 4, 3, 2, 1 and 40, 30, 20, 10 draw the same pie.",
           f"""auto build = [&](double scale) {{
  auto fig = studio("wedge_shares");
  auto& pane = fig.pane2("p");
  pane.wedge(std::vector<double>{{4 * scale, 3 * scale, 2 * scale, 1 * scale}});
  return fig.to_svg();
}};
std::cout << "same pie: " << (build(1.0) == build(10.0)) << "\\n";
{head('wedge_shares_out')}
pane.wedge(std::vector<double>{{40, 30, 20, 10}});
fig.save_svg("figure.svg");"""),
        ex("zero", "A zero value has no wedge, and a pie of one value is a full circle.",
           f"""{head('wedge_zero')}
pane.wedge(std::vector<double>{{5, 0, 3}}, {{"a", "none", "c"}});
fig.save_svg("figure.svg");"""),
        errors("wedge", [("negative value", "pane.wedge(std::vector<double>{1, -2, 3})"), ("all zero", "pane.wedge(std::vector<double>{0, 0})"),
                         ("labels differ", "pane.wedge(std::vector<double>{1, 2, 3}, {\"a\", \"b\"})"), ("explode differs", "pane.wedge(std::vector<double>{1, 2, 3}, std::vector<double>{0, 0.1}, {\"a\", \"b\", \"c\"})")]),
        wrong_kind("wedge", "pane.wedge(std::vector<double>{1, 2, 3})"),
    ]
    write("wedge", dict(
        slug="wedge", title="Pane::wedge",
        signature="template <class Values>\nTrace& Pane::wedge(Values&& values)\ntemplate <class Values>\nTrace& Pane::wedge(Values&& values, std::initializer_list<std::string_view> labels)\ntemplate <class Values, class Explode>\nTrace& Pane::wedge(Values&& values, Explode&& explode, std::initializer_list<std::string_view> labels)",
        intro="""Adds a **pie chart**: one wedge for each value, its angle proportional to the value's share of the total. A pane that holds a pie is drawn without axes.

| Call | Adds |
|---|---|
| `wedge(values)` | Wedges, unlabelled. |
| `wedge(values, labels)` | Wedges with a label each, in order. A range of strings can be given in place of the braces. |
| `wedge(values, explode, labels)` | And a pull-out distance for each wedge: 0 leaves it in place. |

Values must be finite and not negative, with at least one positive, and `labels` and `explode` have one entry for each value. The wedges go clockwise from the top; they take the accent colours of the theme in turn.""",
        parameters="""**values** (`Series`-convertible)
    The sizes of the wedges; only their shares matter.

**explode** (`Series`-convertible, optional)
    How far each wedge is pulled out.

**labels** (strings, optional)
    One label for each wedge.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::columns`](columns.md): the same data as bars.", "[`Pane::radial`](radial.md): polar traces."]))


# ---------------------------------------------------------------- whisk
def whisk():
    ex_ = [
        ex("symmetric", "Values and one error: the same distance above and below each point. The positions are the indices 1, 2, 3, ...",
           f"""{head('whisk_symmetric')}
pane.whisk(std::vector<double>{{3, 5, 4, 6}}, std::vector<double>{{0.5, 1.0, 0.4, 0.8}});
fig.save_svg("figure.svg");"""),
        ex("scalar", "A single error is used for every point.",
           f"""{head('whisk_scalar')}
pane.whisk(std::vector<double>{{3, 5, 4, 6}}, std::vector<double>{{0.7}});
fig.save_svg("figure.svg");"""),
        ex("xy", "With positions: `whisk(x, y, error)`.",
           f"""{head('whisk_xy')}
pane.whisk(std::vector<double>{{1, 2, 4, 8}}, std::vector<double>{{3, 5, 4, 6}}, std::vector<double>{{0.5, 1.0, 0.4, 0.8}});
fig.save_svg("figure.svg");"""),
        ex("asymmetric", "Different errors below and above: `whisk(x, y, below, above)`. Here the lower whiskers are short and the upper ones long.",
           f"""{head('whisk_asymmetric')}
pane.whisk(std::vector<double>{{1, 2, 3, 4}}, std::vector<double>{{3, 5, 4, 6}}, std::vector<double>{{0.2, 0.3, 0.2, 0.4}}, std::vector<double>{{1.0, 1.5, 0.8, 1.2}});
fig.save_svg("figure.svg");"""),
        ex("both", "Errors in both directions: `whisk(x, y, y_below, y_above, x_below, x_above)`. Note the order: the y errors come first.",
           f"""{head('whisk_both')}
pane.whisk(std::vector<double>{{1, 2, 3, 4}}, std::vector<double>{{3, 5, 4, 6}},
           std::vector<double>{{0.5, 0.5, 0.5, 0.5}}, std::vector<double>{{0.5, 0.5, 0.5, 0.5}},
           std::vector<double>{{0.2, 0.3, 0.2, 0.3}}, std::vector<double>{{0.2, 0.3, 0.2, 0.3}});
fig.save_svg("figure.svg");"""),
        ex("with_points", "Whiskers are usually drawn with the points they belong to: a cloud and a whisk of the same data on one pane.",
           f"""{head('whisk_with_points')}
const std::vector<double> x{{1, 2, 3, 4}}, y{{3, 5, 4, 6}};
pane.whisk(x, y, std::vector<double>{{0.5, 1.0, 0.4, 0.8}});
pane.cloud(x, y);
fig.save_svg("figure.svg");"""),
        errors("whisk", [("x and y differ", "pane.whisk(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2}, std::vector<double>{0.1})"),
                         ("error length differs", "pane.whisk(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{0.1, 0.1})"),
                         ("negative error", "pane.whisk(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{-0.1, 0.1, 0.1})")]),
        wrong_kind("whisk", "pane.whisk(std::vector<double>{1, 2}, std::vector<double>{0.1})"),
    ]
    write("whisk", dict(
        slug="whisk", title="Pane::whisk",
        signature="template <class Y, class E>\nTrace& Pane::whisk(Y&& y, E&& err)\ntemplate <class X, class Y, class E>\nTrace& Pane::whisk(X&& x, Y&& y, E&& err)\ntemplate <class X, class Y, class Neg, class Pos>\nTrace& Pane::whisk(X&& x, Y&& y, Neg&& neg, Pos&& pos)\ntemplate <class X, class Y, class YNeg, class YPos, class XNeg, class XPos>\nTrace& Pane::whisk(X&& x, Y&& y, YNeg&& yneg, YPos&& ypos, XNeg&& xneg, XPos&& xpos)",
        intro="""Adds **error bars**: a whisker through each point `(x[i], y[i])` showing the uncertainty of the value. It draws only the whiskers; add the points themselves with [`cloud`](cloud.md) or [`trend`](trend.md).

| Call | Whiskers |
|---|---|
| `whisk(y, err)` | Vertical, `err` above and below, at the positions 1, 2, 3, ... |
| `whisk(x, y, err)` | The same, at the positions `x`. |
| `whisk(x, y, neg, pos)` | Vertical, `neg` below and `pos` above. |
| `whisk(x, y, yneg, ypos, xneg, xpos)` | Vertical and horizontal, each with its own two lengths. The y lengths come first. |

The lengths are distances, not positions: finite and not negative. Each is a series with a length for each point (or one length for all). A rank-2 `y` is read as one series per column.""",
        parameters="""**x**, **y** (`Series`-convertible)
    The points.

**err**, **neg**, **pos**, **yneg**, **ypos**, **xneg**, **xpos** (`Series`-convertible)
    Whisker lengths: finite, not negative, one per point or one for all.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes("- The six-argument call takes the y lengths before the x lengths; passing them the other way compiles and draws the wrong whiskers, so check the figure.\n"),
        see=["[`Pane::cloud`](cloud.md), [`Pane::trend`](trend.md): the points the whiskers belong to.", "[`Pane::band_y`](band_y.md): a shaded interval across the pane."]))


# ---------------------------------------------------------------- fieldmap / contour / isoband
def fieldmap():
    ex_ = [
        ex("grid", "A grid of values drawn as coloured cells: `z(i, j)` is the value at `x[j]` and `y[i]`... here a 3x3 grid of values from 0 to 1.",
           f"""{head('fieldmap_grid')}
{GRID}
pane.fieldmap(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z);
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("smooth", "A smooth field, a hill and a smaller one, on a 25 by 25 grid: the cells are small enough to read as a continuous picture.",
           f"""{head('fieldmap_smooth')}
{BUMP}
pane.fieldmap(axis, axis, z);
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("color", "A fourth argument colours the cells by another quantity: the figure is drawn at the positions of `z`, and coloured by `c`. Here `c` is the field turned upside down, so the colours run the other way.",
           f"""{head('fieldmap_color')}
{GRID}
auto c = pluss::series::linspace(1.0, 0.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}});
auto build = [&](bool with_color) {{
  auto f = studio("fieldmap_color_cmp");
  auto& p = f.pane2("p");
  if (with_color) p.fieldmap(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z, c);
  else p.fieldmap(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z);
  return f.to_svg();
}};
std::cout << "colour series changes the figure: " << (build(true) != build(false)) << "\\n";
pane.fieldmap(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z, c);
fig.save_svg("figure.svg");"""),
        ex("uneven", "The coordinates need not be evenly spaced: the cells are drawn where the coordinates put them.",
           f"""{head('fieldmap_uneven')}
{GRID}
pane.fieldmap(std::vector<double>{{0, 1, 4}}, std::vector<double>{{0, 2, 3}}, z);
fig.save_svg("figure.svg");"""),
        ex("limits", "The range of the colours is the data's own, or what [`color_limits`](color_limits.md) sets. Limits 0..2 over data 0..1 use only the lower half of the palette.",
           f"""auto build = [&](bool limited) {{
  auto fig = studio("fieldmap_limits");
  auto& pane = fig.pane2("p");
  {GRID}
  pane.fieldmap(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z);
  if (limited) pane.color_limits(Range1d{{0.0, 2.0}});
  return fig.to_svg();
}};
std::cout << "limits change the colours: " << (build(true) != build(false)) << "\\n";
{head('fieldmap_limits_out')}
{GRID}
pane.fieldmap(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z);
pane.color_limits(Range1d{{0.0, 2.0}}).scale_guide(true);
fig.save_svg("figure.svg");"""),
        errors("fieldmap", [("coordinates do not match z", "auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3}); pane.fieldmap(std::vector<double>{1, 2}, std::vector<double>{1, 2, 3}, z)"),
                            ("colour has another shape", "auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3}); auto c = pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({2, 2}); pane.fieldmap(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, z, c)")]),
        wrong_kind("fieldmap", "pane.fieldmap(std::vector<double>{1, 2}, std::vector<double>{1, 2}, pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({2, 2}))"),
    ]
    write("fieldmap", dict(
        slug="fieldmap", title="Pane::fieldmap",
        signature="template <class X, class Y, class Z>\nTrace& Pane::fieldmap(X&& x, Y&& y, Z&& z)\ntemplate <class X, class Y, class Z, class C>\nTrace& Pane::fieldmap(X&& x, Y&& y, Z&& z, C&& c)",
        intro="""Adds a **heat map**: a grid of cells coloured by the value of a field `z` at each grid point. It shows a quantity that depends on two others, a measurement over a plane, a table of values, a correlation matrix.

`z` is a rank-2 matrix with one row for each `y` and one column for each `x`; `x` and `y` give the coordinates of the columns and rows (or, as matrices, of every point). They need not be evenly spaced. The colours come from the palette of the trace over the range of the data, or the pane's [`color_limits`](color_limits.md).

| Call | Colours from |
|---|---|
| `fieldmap(x, y, z)` | `z`. |
| `fieldmap(x, y, z, c)` | `c`, a matrix of the same shape as `z`. The cells are placed by `z`'s grid and coloured by another quantity. |

Switch on [`scale_guide`](scale_guide.md) to show what the colours mean. The contours of the same kind of data are drawn by [`contour`](contour.md) and [`isoband`](isoband.md); in three dimensions use [`sheet`](sheet.md).""",
        parameters="""**x**, **y** (`Series`-convertible)
    The coordinates of the columns and the rows, or full matrices of coordinates.

**z** (`Series`-convertible, rank 2)
    The field.

**c** (`Series`-convertible, optional)
    The values that colour the cells, with as many elements as `z`.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::contour`](contour.md), [`Pane::isoband`](isoband.md): the same data as lines or bands.", "[`Pane::sheet`](sheet.md): the field as a surface.", "[`Pane::scale_guide`](scale_guide.md): the key to the colours."]))


def contour_like(slug, filled):
    verb = "bands" if filled else "lines"
    ex_ = [
        ex("default", f"The default: 8 evenly spaced levels between the lowest and the highest value of the field, drawn as {verb}.",
           f"""{head(slug + '_default')}
{BUMP}
pane.{slug}(axis, axis, z);
fig.save_svg("figure.svg");"""),
        ex("count", "A count of levels, as an integer, spaces that many levels evenly. Three levels give a coarse picture.",
           f"""{head(slug + '_count')}
{BUMP}
pane.{slug}(axis, axis, z, 3);
fig.save_svg("figure.svg");"""),
        ex("levels", "Explicit levels, in a container, are used as given: here the levels at 0.1, 0.5 and 0.9 of a field that runs from 0 to about 1.2.",
           f"""{head(slug + '_levels')}
{BUMP}
pane.{slug}(axis, axis, z, std::vector<double>{{0.1, 0.5, 0.9}});
fig.save_svg("figure.svg");"""),
        ex("integer_or_container", "An integer is a count and a container is a list of levels: `4` and `std::vector<double>{4}` are different calls. The first draws four levels, the second one level at the value 4, which a field below 4 does not reach.",
           f"""auto build = [&](bool count) {{
  auto fig = studio("{slug}_int_or_container");
  auto& pane = fig.pane2("p");
  {BUMP}
  if (count) pane.{slug}(axis, axis, z, 4);
  else pane.{slug}(axis, axis, z, std::vector<double>{{4.0}});
  return fig.to_svg();
}};
std::cout << "count and list differ: " << (build(true) != build(false)) << "\\n";
{head(slug + '_int_or_container_out')}
{BUMP}
pane.{slug}(axis, axis, z, 4);
fig.save_svg("figure.svg");"""),
        ex("with_map", f"Over a heat map of the same field the {verb} mark where the colours change. Both traces are on one pane, the {'bands' if filled else 'lines'} drawn after the map.",
           f"""{head(slug + '_with_map')}
{BUMP}
pane.fieldmap(axis, axis, z);
pane.{slug}(axis, axis, z, 6);
fig.save_svg("figure.svg");"""),
        ex("uneven", "The coordinates can be uneven: the grid is stretched to them.",
           f"""{head(slug + '_uneven')}
{GRID}
pane.{slug}(std::vector<double>{{0, 1, 4}}, std::vector<double>{{0, 2, 3}}, z, 4);
fig.save_svg("figure.svg");"""),
        errors(slug, [("no levels", f"auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}}); pane.{slug}(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z, 0)"),
                      ("negative levels", f"auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}}); pane.{slug}(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z, -1)"),
                      ("coordinates do not match z", f"auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}}); pane.{slug}(std::vector<double>{{1, 2}}, std::vector<double>{{1, 2, 3}}, z)"),
                      ("z is not a matrix", f"pane.{slug}(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}})")]),
        wrong_kind(slug, f"pane.{slug}(std::vector<double>{{1, 2}}, std::vector<double>{{1, 2}}, pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({{2, 2}}))"),
    ]
    write(slug, dict(
        slug=slug, title=f"Pane::{slug}",
        signature=f"template <class X, class Y, class Z>\nTrace& Pane::{slug}(X&& x, Y&& y, Z&& z)\ntemplate <class X, class Y, class Z, class Levels>\nTrace& Pane::{slug}(X&& x, Y&& y, Z&& z, Levels&& levels)\ntemplate <class X, class Y, class Z>\nTrace& Pane::{slug}(X&& x, Y&& y, Z&& z, int level_count)",
        intro=f"""Adds {'**filled bands**: the regions between successive levels of a field, each filled with its own colour' if filled else '**contour lines**: curves along which a field `z(x, y)` has the same value, one curve family for each level'}. {'It is the filled form of [`contour`](contour.md).' if filled else 'Together with a [heat map](fieldmap.md) it is the usual way to show a quantity over a plane.'}

`z` is a rank-2 matrix with a row for each `y` and a column for each `x`, as for [`fieldmap`](fieldmap.md). The levels are chosen by the call:

| Call | Levels |
|---|---|
| `{slug}(x, y, z)` | 8, evenly spaced between the lowest and highest value of `z`. |
| `{slug}(x, y, z, level_count)` | `level_count` (an integer, positive), evenly spaced. |
| `{slug}(x, y, z, levels)` | Exactly the values in the container `levels`. |

An integer is always a count: `4` asks for four levels, and one level at the value 4 is `std::vector<double>{{4.0}}`. The colours of the {verb} come from the palette over the range of the field (or [`color_limits`](color_limits.md)); with one level the trace's own colour is used.""",
        parameters=f"""**x**, **y** (`Series`-convertible)
    The coordinates of the columns and rows, or full matrices.

**z** (`Series`-convertible, rank 2)
    The field.

**level_count** (`int`) or **levels** (container of numbers)
    How many levels, or which.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=[f"[`Pane::{'contour' if filled else 'isoband'}`]({'contour' if filled else 'isoband'}.md): the {'lines' if filled else 'filled bands'}.", "[`Pane::fieldmap`](fieldmap.md): the heat map.", "[`Pane::ridge3`](ridge3.md): contour lines lifted into three dimensions.", "[`Trace::annotate_levels`](../trace/annotate_levels.md): write the level on each line."]))


def main():
    trend(); cloud(); columns(False); columns(True); bucket(); quartet(); violin(); radial()
    ribbon_spike("ribbon"); ribbon_spike("spike"); wedge(); whisk(); fieldmap()
    contour_like("contour", False); contour_like("isoband", True)
    print("wrote planar plot specs")


if __name__ == "__main__":
    main()
