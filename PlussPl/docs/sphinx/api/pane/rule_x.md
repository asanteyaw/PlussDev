<!-- pluss-hand-authored -->

# Pane::rule_x

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::rule_x(double value, std::string label = {})
```

## What It Does

Adds a reference line at x = `value`: a rule drawn across the plane at that position, with an optional `label` written on it. Rules mark thresholds, targets and baselines without being data: they are not traces, they are not in the legend, and they take the colours of the pane's theme. Each call adds one rule; they are kept in the order added and listed by [`rules`](rules.md).

The position must be finite; a NaN or infinite value throws `pluss::Error` and nothing is added. A rule takes part in the automatic range of its axis, so a rule outside the data widens the axis to keep it visible, while a manual range ([`x_limits`](x_limits.md)) is never changed by a rule, and a rule outside it is not drawn. The rule is drawn when the figure is rendered, so it can be added before or after the traces.

## Parameters

**value** (`double`)
    The position of the rule, in data units. Finite.

**label** (`std::string`, default empty)
    Text written on the rule. Empty draws no text.

## Returns

`Pane&`
    The same pane, for chaining.

## Examples

Example 1. A labelled reference line. A rule at x = 2 with the label `target`; it is stored with its axis, value and label, and the label is drawn on the line.

```cpp
auto fig = studio("rule_x_basic");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.rule_x(2.0, "target");
const auto& rule = pane.rules().front();
std::cout << "stored: axis " << static_cast<int>(rule.axis) << ", value " << rule.value << ", label '" << rule.label << "', width " << rule.width << "\n";
std::cout << "label drawn: " << (fig.to_svg().find(">target<") != std::string::npos) << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
stored: axis 0, value 2, label 'target', width 1.15
label drawn: 1
```

![Output of Example 1](img/rule_x-1-basic.svg)

Example 2. Without a label. The label is optional: the line is drawn and no extra text appears, and the figure has one more path than the one without the rule.

```cpp
auto paths = [](const std::string& svg) {
  std::size_t n = 0;
  for (auto at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) ++n;
  return n;
};
auto build = [&](bool with_rule) {
  auto fig = studio("rule_x_nolabel");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
  if (with_rule) pane.rule_x(2.0);
  return fig.to_svg();
};
std::cout << "paths without: " << paths(build(false)) << ", with: " << paths(build(true)) << "\n";
auto fig = studio("rule_x_nolabel_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.rule_x(2.0);
fig.save_svg("figure.svg");
```

### Output

```text
paths without: 19, with: 20
```

![Output of Example 2](img/rule_x-2-no_label.svg)

Example 3. Several rules. Each call adds a rule; they are kept in the order added and each is drawn, so a pane can carry a baseline and thresholds together.

```cpp
auto fig = studio("rule_x_several");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.rule_x(1.5, "low").rule_x(2.5, "mid").rule_x(3.5, "high");
std::cout << "rules: " << pane.rules().size() << ":";
for (const auto& r : pane.rules()) std::cout << " " << r.label << "@" << r.value;
std::cout << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
rules: 3: low@1.5 mid@2.5 high@3.5
```

![Output of Example 3](img/rule_x-3-several.svg)

Example 4. Rules and the x range. A rule far outside the data widens an automatic range so it stays visible; with a manual range the axis stays where it was set, and a rule outside that range is not drawn at all.

```cpp
auto texts = [](const std::string& svg) {
  std::string out;
  for (std::size_t at = svg.find("<text"); at != std::string::npos; at = svg.find("<text", at + 1)) {
    const auto gt = svg.find('>', at);
    out += svg.substr(gt + 1, svg.find("</text>", gt) - gt - 1) + " ";
  }
  return out;
};
auto build = [&](bool manual) {
  auto fig = studio("rule_x_range");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
  pane.y_tick_labels(false);
  pane.z_tick_labels(false);
  if (manual) pane.x_limits(Range1d{0.0, 5.0});
  pane.rule_x(40.0, "far");
  return fig.to_svg();
};
std::cout << "automatic range: " << texts(build(false)) << "\n";
std::cout << "manual 0 to 5:   " << texts(build(true)) << "\n";
auto fig = studio("rule_x_range_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.rule_x(40.0, "far");
fig.save_svg("figure.svg");
```

### Output

```text
automatic range: p 0 10 20 30 40 far 
manual 0 to 5:   p 0 1 2 3 4 5 
```

![Output of Example 4](img/rule_x-4-range.svg)

Example 5. Only finite values. A NaN or infinite position throws `pluss::Error` and no rule is added.

```cpp
auto fig = studio("rule_x_invalid");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
for (double bad : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}) {
  try {
    pane.rule_x(bad, "never");
  } catch (const pluss::Error& e) {
    std::cout << bad << ": " << e.message() << "\n";
  }
}
std::cout << "rules added: " << pane.rules().size() << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
nan: rule_x: value must be finite
inf: rule_x: value must be finite
-inf: rule_x: value must be finite
rules added: 0
```

![Output of Example 5](img/rule_x-5-invalid.svg)

## Notes

- Not `noexcept`: it throws for a non-finite value.
- A rule marks one position; for a range use [`band_x`](band_x.md).

## See Also

- [`Pane::band_x`](band_x.md): a shaded range.
- [`Pane::rules`](rules.md): the rules of the pane.
- [`Pane::pin`](pin.md): a note attached to a point.
- [`Pane::rule_y`](rule_y.md): the same for the y axis.
- [`Pane::rule_z`](rule_z.md): the same for the z axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ridge3.html">
    <span class="label">Previous</span>
    <span class="title">ridge3</span>
  </a>
  <a class="next-link" href="rule_y.html">
    <span class="label">Next</span>
    <span class="title">rule_y</span>
  </a>
</div>
