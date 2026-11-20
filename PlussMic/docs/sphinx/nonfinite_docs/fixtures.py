"""C++ fixtures and printing idioms for the NULL / NaN / infinity API pages.

Every page shows complete, runnable code. The same six-row column is used on every type, so a reader who has seen one page
recognises the data on all the others:

    row   0     1     2     3     4      5
          1    NaN   +inf  -inf  NULL    5

Row 4 is NULL (the validity mask marks it missing; the 0.0 underneath is only a placeholder). Rows 1-3 hold values that
are not ordinary numbers. A second feature / stream / field ("b") has NaN in rows 1 and 2, so the any / all rule of the
two-dimensional types has something to show.
"""

A_VALUES = "{1.0, nan, inf, -inf, 0.0, 5.0}"
A_MASK = "std::vector<bool>{true, true, true, true, false, true}"
B_VALUES = "{10.0, nan, nan, 40.0, 50.0, 60.0}"
CLEAN_A = "{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}"
CLEAN_B = "{10.0, 20.0, 30.0, 40.0, 50.0, 60.0}"
INT_A = "std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}"
INT_B = "std::vector<std::int64_t>{10, 20, 30, 40, 50, 60}"

PREAMBLE_CONSTS = [
    "const double nan = std::numeric_limits<double>::quiet_NaN();",
    "const double inf = std::numeric_limits<double>::infinity();",
]


def series(values: str) -> str:
    return f"pluss::make_series({values})" if values.startswith("{") else f"pluss::make_series({values})"


# ---- the one-dimensional unit of each family ---------------------------------------------------------------------------------

def shelf_expr(kind: str, col: str) -> str:
    """A Shelf. kind: base | clean | ints. col: a | b."""
    if kind == "base":
        if col == "a":
            return "Shelf(values, present)"
        return f"Shelf({series(B_VALUES)})"
    if kind == "clean":
        return f"Shelf({series(CLEAN_A if col == 'a' else CLEAN_B)})"
    return f"Shelf({series(INT_A if col == 'a' else INT_B)})"


def stream_expr(kind: str, col: str) -> str:
    if kind == "base":
        if col == "a":
            return "Stream(values, present)"
        return f"Stream({series(B_VALUES)})"
    if kind == "clean":
        return f"Stream({series(CLEAN_A if col == 'a' else CLEAN_B)})"
    return f"Stream({series(INT_A if col == 'a' else INT_B)})"


def slot_cells(kind: str, col: str) -> list[str]:
    if kind == "base":
        if col == "a":
            return ["pluss::make_series({1.0})", "pluss::make_series({nan})", "pluss::make_series({inf})",
                    "pluss::make_series({-inf})", "std::monostate{}", "pluss::make_series({5.0})"]
        return ["pluss::make_series({10.0})", "pluss::make_series({nan})", "pluss::make_series({nan})",
                "pluss::make_series({40.0})", "pluss::make_series({50.0})", "pluss::make_series({60.0})"]
    vals = [1, 2, 3, 4, 5, 6] if col == "a" else [10, 20, 30, 40, 50, 60]
    if kind == "clean":
        return [f"pluss::make_series({{{v}.0}})" for v in vals]
    return [f"pluss::make_series(std::vector<std::int64_t>{{{v}}})" for v in vals]


def slot_lines(var: str, kind: str, col: str) -> list[str]:
    cells = slot_cells(kind, col)
    return [f"auto {var} = Slot({{", f"    {cells[0]}, {cells[1]}, {cells[2]},", f"    {cells[3]}, {cells[4]}, {cells[5]}}});"]


# ---- type descriptions ------------------------------------------------------------------------------------------------------

class T:
    """One documented type: how to build it, how to print it, and the words used about it."""

    def __init__(self, name, dirname, usings, noun, plural, cls_title, units=None):
        self.name = name
        self.dirname = dirname
        self.usings = usings
        self.noun = noun            # "entry", "row", "cell" ...
        self.plural = plural
        self.title = cls_title
        self.units = units          # for stacks / workspaces: the name of one member


def build(t: T, kind: str, indent: str = "") -> list[str]:
    """Lines that define `x` for variant kind (base | clean | ints)."""
    n = t.name
    pre = []
    if kind == "base" and n not in ("Slot", "Registry", "Axel"):
        pre = [f"auto values = pluss::make_series({A_VALUES});", f"auto present = pluss::make_series({A_MASK});"]
    return pre + _build(t, kind)


def _build(t: T, kind: str) -> list[str]:
    n = t.name
    if n == "Shelf":
        return [f"auto x = {shelf_expr(kind, 'a')};"]
    if n == "Stream":
        return [f"auto x = {stream_expr(kind, 'a')};"]
    if n == "Slot":
        return slot_lines("x", kind, "a")
    if n == "DataFrame":
        return df_lines("x", kind)
    if n == "Catalog":
        return cat_lines("x", kind)
    if n == "Registry":
        return reg_lines("x", kind)
    second = "clean" if kind == "base" else kind
    if n == "Atlas":
        return df_lines("frame", kind) + df_lines("other", second) + ["auto x = Atlas(std::vector<DataFrame>{frame, other});"]
    if n == "Nexus":
        return df_lines("a", kind) + df_lines("b", second) + ['auto x = Nexus({{"a", a}, {"b", b}});']
    if n == "Archive":
        return cat_lines("first", kind) + cat_lines("second", second) + ["auto x = Archive(std::vector<Catalog>{first, second});"]
    if n == "Locus":
        return cat_lines("a", kind) + cat_lines("b", second) + [
            'auto x = Locus({{"a", a}, {"b", b}});']
    if n == "Axel":
        return reg_lines("first", kind) + reg_lines("second", second) + ["auto x = Axel(std::vector<Registry>{first, second});"]
    raise KeyError(n)


def df_lines(var: str, kind: str) -> list[str]:
    return [f"auto {var} = DataFrame({{",
            f'    {{"a", {shelf_expr(kind, "a")}}},',
            f'    {{"b", {shelf_expr(kind, "b")}}},',
            "});"]


def cat_lines(var: str, kind: str) -> list[str]:
    return [f"auto {var} = Catalog(std::vector<std::pair<std::string, Stream>>{{",
            f'    {{"a", {stream_expr(kind, "a")}}},',
            f'    {{"b", {stream_expr(kind, "b")}}},',
            "});"]


def reg_lines(var: str, kind: str) -> list[str]:
    return (slot_lines(f"{var}_a", kind, "a") + slot_lines(f"{var}_b", kind, "b")
            + [f'auto {var} = Registry({{{{"a", {var}_a}}, {{"b", {var}_b}}}});'])


SLOT_HELPER = [
    "// A slot cell holds a Series; print a slot of one-number cells the way a column prints (an empty cell is NULL, a true/false cell is 1/0).",
    "auto column = [](const Slot& slot) {",
    "    std::vector<double> values;",
    "    std::vector<bool> present;",
    "    for (std::int64_t i = 0; i < slot.size(); ++i) {",
    "        const auto& cell = slot.at(i);",
    "        present.push_back(!std::holds_alternative<std::monostate>(cell));",
    "        values.push_back(present.back() ? std::get<pluss::Series>(cell).to(pluss::codem::ScalarType::Float64).host<double>()[0] : 0.0);",
    "    }",
    "    return pluss::table::Shelf(pluss::make_series(values), pluss::make_series(present));",
    "};",
]


def show(t: T, expr: str, label: str | None = None) -> list[str]:
    """Statements that print `expr` (a value of type t, or - for count helpers - anything printable)."""
    pre = [f'std::cout << "{label}\\n";'] if label else []
    n = t.name
    if n in ("Shelf", "DataFrame", "Stream", "Atlas"):
        body = f'std::cout << {expr} << "\\n";'
    elif n == "Catalog":
        body = f'std::cout << {expr}.keys("a") << "\\n" << {expr}.keys("b") << "\\n";'
    elif n == "Archive":
        body = f'std::cout << {expr}.catalog(0).keys("a") << "\\n" << {expr}.catalog(1).keys("a") << "\\n";'
    elif n == "Slot":
        body = f'std::cout << column({expr}) << "\\n";'
    elif n == "Registry":
        body = f'std::cout << column({expr}.fields("a")) << "\\n" << column({expr}.fields("b")) << "\\n";'
    elif n == "Axel":
        body = f'std::cout << column({expr}.registry(0).fields("a")) << "\\n" << column({expr}.registry(1).fields("a")) << "\\n";'
    elif n == "Nexus":
        body = f'std::cout << {expr}.frame("a") << "\\n" << {expr}.frame("b") << "\\n";'
    elif n == "Locus":
        body = f'std::cout << {expr}.catalog("a").keys("a") << "\\n" << {expr}.catalog("b").keys("a") << "\\n";'
    else:
        raise KeyError(n)
    return pre + [body]


TYPES = [
    T("Shelf", "shelf", ["using namespace pluss::table;"], "entry", "entries", "Shelf"),
    T("DataFrame", "dataframe", ["using namespace pluss::table;"], "row", "rows", "DataFrame"),
    T("Atlas", "atlas", ["using namespace pluss::table;"], "row", "rows", "Atlas", units="frame"),
    T("Stream", "stream", ["using namespace pluss::kindle;"], "record", "records", "Stream"),
    T("Catalog", "catalog", ["using namespace pluss::kindle;"], "record", "records", "Catalog"),
    T("Archive", "archive", ["using namespace pluss::kindle;"], "record", "records", "Archive", units="catalog"),
    T("Slot", "slot", ["using namespace pluss::scribble;"], "cell", "cells", "Slot"),
    T("Registry", "registry", ["using namespace pluss::scribble;"], "cell", "cells", "Registry"),
    T("Axel", "axel", ["using namespace pluss::scribble;"], "cell", "cells", "Axel", units="registry"),
    T("Nexus", "nexus", ["using namespace pluss::bundle;", "using namespace pluss::table;"], "row", "rows", "Nexus", units="table"),
    T("Locus", "locus", ["using namespace pluss::bundle;", "using namespace pluss::kindle;"], "record", "records", "Locus", units="catalog"),
]
BY_NAME = {t.name: t for t in TYPES}
