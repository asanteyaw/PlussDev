from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / "advanced_thread_api_reference.md"


@dataclass(frozen=True)
class Api:
    owner: str
    name: str
    args: str
    result: str
    notes: str
    call: str
    expected: str


APIS = [
    Api("Thread", "map_elements", "fn: function<ThreadValue(const ThreadValue&)>", "Thread", "Calls a scalar callback for every value and preserves one-dimensional ordering.", "numbers.map_elements(double_value)", "[2.0, 4.0, 6.0]"),
    Api("Thread", "aggregate_with", "fn: function<Thread(const Thread&)>, shape: ResultShape = Reduce", "Thread", "Runs an arbitrary Thread reduction or shape-controlled transformation.", "numbers.aggregate_with(total_value)", "[6.0]"),
    Api("Thread", "flow", "fn: function<Thread(const Thread&)>", "Thread", "Passes the complete Thread through a chainable callback.", "numbers.flow(center_values)", "[-1.0, 0.0, 1.0]"),
    Api("DataFrame", "map_cases", "fn: function<ThreadValue(const DataFrame&)>", "Thread", "Calls a scalar callback once per case and returns one ordered Thread.", "df.map_cases(score_case)", "[10.0, 20.0, 30.0]"),
    Api("DataFrame", "apply_cases", "fn: function<DataFrame(const DataFrame&)>, shape: ResultShape = Infer", "DataFrame", "Applies a frame-returning callback per case with explicit inferred, reduced, preserved, or expanded shape checks.", "df.apply_cases(identity_case, ResultShape::Preserve)", "the original three-case frame"),
    Api("DataFrame", "map_elements", "fn: function<ThreadValue(const ThreadValue&)>, features: RangeView<string_view> = {}", "DataFrame", "Delegates element mapping to every selected feature Thread.", "numeric.map_elements(double_value, {\"x\", \"y\"})", "every selected numeric cell is doubled"),
    Api("DataFrame", "aggregate_with", "fn: function<Thread(const DataFrame&)>, shape: ResultShape = Reduce", "Thread", "Runs an arbitrary whole-frame callback and validates the requested result shape.", "df.aggregate_with(total_x)", "[6.0]"),
    Api("DataFrame", "flow", "fn: function<DataFrame(const DataFrame&)>", "DataFrame", "Passes the complete frame through a chainable callback.", "df.flow(clean_labels)", "label = [A, BB, CCC]"),
    Api("Atlas", "map_elements", "fn: function<ThreadValue(const ThreadValue&)>", "Atlas", "Delegates element mapping through DataFrame and Thread for every depth frame.", "atlas.map_elements(double_value)", "every compatible cell in both frames is doubled"),
    Api("AtlasTsAccessor", "rolling", "window: int64, statistic: WindowStatistic = Mean, options: WindowOptions = {}", "Atlas", "Delegates the selected rolling statistic through `atlas.ts(...)` to every DataFrame at depth.", "atlas.ts({\"x\"}).rolling(2, WindowStatistic::Sum)", "each frame x = [1.0, 3.0, 5.0]"),
    Api("Atlas", "flow", "fn: function<Atlas(const Atlas&)>", "Atlas", "Passes the complete Atlas through a chainable callback.", "atlas.flow(clean_each_frame)", "labels are uppercased in every frame"),
    Api("Thread", "upper", "", "Thread", "Converts every observed text value to Unicode uppercase.", "text.upper()", "[ALPHA-10, BETA-20, RED|BLUE]"),
    Api("Thread", "casefold", "", "Thread", "Applies Unicode case folding for caseless matching.", "unicode.casefold()", "[café, strasse]"),
    Api("Thread", "title_case", "", "Thread", "Converts observed text to title case.", "words.title_case()", "[North America, West Africa]"),
    Api("Thread", "text_length", "", "Thread", "Counts Unicode code points in every observed text value.", "text.text_length()", "[8, 7, 8]"),
    Api("Thread", "starts_with", "prefix: string_view", "Thread", "Tests whether each observed text value begins with a prefix.", "text.starts_with(\"alpha\")", "[true, false, false]"),
    Api("Thread", "ends_with", "suffix: string_view", "Thread", "Tests whether each observed text value ends with a suffix.", "text.ends_with(\"20\")", "[false, true, false]"),
    Api("Thread", "regex_match", "pattern: string_view", "Thread", "Tests an ECMAScript regular expression at the beginning of each text value.", "text.regex_match(\"[a-z]+\")", "[true, false, true]"),
    Api("Thread", "regex_fullmatch", "pattern: string_view", "Thread", "Tests whether an ECMAScript regular expression consumes each complete text value.", "text.regex_fullmatch(\"[A-Za-z]+-[0-9]+\")", "[true, true, false]"),
    Api("Thread", "regex_extract", "pattern: string_view, names: RangeView<string_view> = {}", "DataFrame", "Extracts every ECMAScript capture group into a named feature.", "text.regex_extract(\"([A-Za-z]+)-([0-9]+)\", {\"word\", \"number\"})", "word = [alpha, Beta, NA]; number = [10, 20, NA]"),
    Api("Thread", "split_text", "delimiter: string_view, limit: int64 = -1, prefix: string = \"part\"", "DataFrame", "Splits each text value into a rectangular set of nullable output features.", "text.split_text(\"-\")", "part_0 = [alpha, Beta, red|blue]; part_1 = [10, 20, NA]"),
    Api("Thread", "partition_text", "separator: string_view, prefix: string = \"part\"", "DataFrame", "Partitions each value into before, separator, and after features.", "text.partition_text(\"-\")", "the first row is [alpha, -, 10]"),
    Api("Thread", "pad_text", "width: int64, align: TextAlign = Left, fill: char = ' '", "Thread", "Pads text to a minimum character width with typed alignment.", "text.pad_text(10, TextAlign::Right, '.')", "[..alpha-10, ...Beta-20, ..red|blue]"),
    Api("Thread", "slice_text", "start: int64, stop: optional<int64> = nullopt, step: int64 = 1", "Thread", "Slices each value by Unicode character position.", "text.slice_text(0, 5)", "[alpha, Beta-, red|b]"),
    Api("Thread", "normalize_text", "form: UnicodeForm = NFC", "Thread", "Applies canonical or compatibility Unicode normalization.", "unicode.normalize_text(UnicodeForm::NFC)", "[Café, Straße]"),
    Api("Thread", "encode_text", "encoding: string_view = \"base64\"", "Thread", "Encodes each value as base64, hexadecimal, or URL text.", "text.encode_text(\"hex\")", "[616c7068612d3130, 426574612d3230, 7265647c626c7565]"),
    Api("Thread", "text_indicators", "delimiter: string_view = \"|\", prefix: string = \"\"", "DataFrame", "Creates one boolean feature per distinct delimited token.", "text.text_indicators(\"|\")", "red and blue are true for the third case"),
    Api("DataFrame", "text_upper", "features: RangeView<string_view> = {}", "DataFrame", "Delegates Unicode uppercase conversion to selected text Threads.", "df.text_upper({\"label\"})", "label = [A, BB, CCC]"),
    Api("DataFrame", "text_casefold", "features: RangeView<string_view> = {}", "DataFrame", "Delegates Unicode case folding to selected text Threads.", "df.text_casefold({\"label\"})", "label contains caseless normalized forms"),
    Api("DataFrame", "text_title", "features: RangeView<string_view> = {}", "DataFrame", "Delegates title casing to selected text Threads.", "df.text_title({\"label\"})", "selected labels use title case"),
    Api("DataFrame", "text_lengths", "features: RangeView<string_view> = {}", "DataFrame", "Replaces selected text features with Unicode character counts.", "df.text_lengths({\"label\"})", "label = [1, 2, 3]"),
    Api("DataFrame", "text_starts_with", "prefix: string_view, features: RangeView<string_view> = {}", "DataFrame", "Delegates prefix testing to selected text Threads.", "df.text_starts_with(\"a\", {\"label\"})", "label = [true, false, false]"),
    Api("DataFrame", "text_ends_with", "suffix: string_view, features: RangeView<string_view> = {}", "DataFrame", "Delegates suffix testing to selected text Threads.", "df.text_ends_with(\"c\", {\"label\"})", "label = [false, false, true]"),
    Api("DataFrame", "text_matches", "pattern: string_view, full: bool = false, features: RangeView<string_view> = {}", "DataFrame", "Delegates prefix or full regular-expression matching to selected text Threads.", "df.text_matches(\"[a-z]+\", true, {\"label\"})", "each selected cell reports a full-match boolean"),
    Api("DataFrame", "text_pad", "width: int64, align: TextAlign = Left, fill: char = ' ', features: RangeView<string_view> = {}", "DataFrame", "Delegates text padding to selected text Threads.", "df.text_pad(4, TextAlign::Right, '.', {\"label\"})", "label = [...a, ..bb, .ccc]"),
    Api("DataFrame", "text_slice", "start: int64, stop: optional<int64> = nullopt, step: int64 = 1, features: RangeView<string_view> = {}", "DataFrame", "Delegates Unicode character slicing to selected text Threads.", "df.text_slice(0, 1, 1, {\"label\"})", "label = [a, b, c]"),
    Api("DataFrame", "text_normalize", "form: UnicodeForm = NFC, features: RangeView<string_view> = {}", "DataFrame", "Delegates Unicode normalization to selected text Threads.", "df.text_normalize(UnicodeForm::NFC, {\"label\"})", "canonically equivalent labels use NFC encoding"),
    Api("DataFrame", "text_encode", "encoding: string_view = \"base64\", features: RangeView<string_view> = {}", "DataFrame", "Delegates text encoding to selected text Threads.", "df.text_encode(\"hex\", {\"label\"})", "label = [61, 6262, 636363]"),
    Api("DataFrame", "regex_groups", "feature: string_view, pattern: string_view, names: RangeView<string_view> = {}", "DataFrame", "Expands capture groups from one text feature while preserving cases.", "people.regex_groups(\"code\", \"([A-Z]+)-([0-9]+)\", {\"site\", \"id\"})", "site and id contain the two capture groups"),
    Api("DataFrame", "split_feature", "feature: string_view, delimiter: string_view, limit: int64 = -1, prefix: string = \"part\"", "DataFrame", "Expands one delimited text feature while preserving cases.", "people.split_feature(\"code\", \"-\")", "part_0 and part_1 contain the split code"),
    Api("DataFrame", "partition_feature", "feature: string_view, separator: string_view, prefix: string = \"part\"", "DataFrame", "Expands one text feature into before, separator, and after features.", "people.partition_feature(\"code\", \"-\")", "three partition features preserve the source cases"),
    Api("DataFrame", "text_indicators", "feature: string_view, delimiter: string_view = \"|\", prefix: string = \"\"", "DataFrame", "Expands delimited categories into boolean indicator features.", "people.text_indicators(\"tags\", \"|\")", "one boolean feature is returned for every observed tag"),
]


def split_args(args: str) -> list[str]:
    if not args:
        return []
    depth = 0
    current = []
    output = []
    for char in args + ",":
        depth += char in "<({"
        depth -= char in ">)}"
        if char == "," and depth == 0:
            output.append("".join(current).strip())
            current = []
        else:
            current.append(char)
    return output


def examples(api: Api) -> list[tuple[str, str]]:
    return [
        (f"auto out = {api.call};", api.expected),
        (f"auto out = nullable_{api.call};", "null input positions remain null; observed positions follow the same contract"),
        (f"auto out = one_{api.call};", "one input value or case produces one contract-compatible result"),
        (f"auto out = {api.call}.flow([](const auto& value) {{ return value; }});", "identity flow preserves the preceding result"),
        (f"auto first = {api.call};\nauto second = {api.call};", "repeated calls are deterministic and produce identical values and validity"),
    ]


def render() -> str:
    lines = [
        "# Advanced Thread, UDF, Text, and Delegation API Reference",
        "",
        "Outputs on this page are exercised by `plussmic_table_statistics_window_tests`. "
        "Built-in numeric work stays in Series kernels; scalar and case callbacks are explicit UDF boundaries.",
        "",
        "Full Unicode normalization and case conversion use the Unicode backend discovered while building the SDK. "
        "A build without that backend still handles ASCII exactly and fails explicitly for non-ASCII input.",
        "",
    ]
    for api in APIS:
        lines.extend([
            f"## `{api.owner}::{api.name}`",
            "",
            "### API Signature",
            "",
            "```text",
            f"{api.owner}::{api.name}({api.args}) -> {api.result}",
            "```",
            "",
            "### Notes",
            "",
            api.notes,
            "",
            "### Inputs",
            "",
        ])
        args = split_args(api.args)
        if not args:
            lines.extend(["`None`", ""])
        for arg in args:
            name = arg.split(":", 1)[0]
            lines.extend([
                f"`{arg}`",
                f"    Public value controlling `{name}`. Invalid values fail through the Pluss error system.",
                "",
            ])
        lines.extend([
            "### Output",
            "",
            f"`{api.result}` with the shape and validity described above.",
            "",
            "### Options",
            "",
            "`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; "
            "`TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports "
            "`NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.",
            "",
            "### Examples",
            "",
        ])
        for index, (code, output) in enumerate(examples(api), 1):
            lines.extend([
                f"Example {index}.",
                "",
                "```cpp",
                code,
                "```",
                "",
                "#### Expected output",
                "",
                "```text",
                output,
                "```",
                "",
            ])
    return "\n".join(lines)


if __name__ == "__main__":
    OUTPUT.write_text(render())
    print(f"wrote {OUTPUT}")
