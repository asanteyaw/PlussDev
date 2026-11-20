"""What each NULL / NaN / infinity method does, in words and in examples.

`page_spec(type, method)` returns everything one API page needs except the example outputs, which the runner fills in from a
real run. Prose is written once per method and adapted per type through the small vocabulary in `fixtures.T` and the
per-type statements below, so a reader who has learned one page can predict every other page.
"""
from __future__ import annotations

import re
from fixtures import BY_NAME, PREAMBLE_CONSTS, SLOT_HELPER, T, build, show

CELLS = [("An ordinary number (`1`, `5`)", "ord"), ("`NaN`", "nan"), ("`+inf`", "pinf"), ("`-inf`", "ninf"), ("`NULL` (missing)", "null")]

FLAG = {  # method -> (what it asks, results for ord, nan, +inf, -inf, NULL)
    "isnan": ("holds a `NaN`", "F T F F F"),
    "notnan": ("does **not** hold a `NaN`", "T F T T T"),
    "isinf": ("holds an infinity (`+inf` or `-inf`)", "F F T T F"),
    "notinf": ("does **not** hold an infinity", "T T F F T"),
    "isposinf": ("holds `+inf`", "F F T F F"),
    "isneginf": ("holds `-inf`", "F F F T F"),
}
DROP = {  # method -> kind dropped: set of cell classes removed
    "dropnan": ({"nan"}, "NaN"),
    "dropinf": ({"pinf", "ninf"}, "infinity"),
    "dropposinf": ({"pinf"}, "+inf"),
    "dropneginf": ({"ninf"}, "-inf"),
    "dropinvalid": ({"nan", "pinf", "ninf", "null"}, "invalid"),
}
FILL = {  # method -> (classes replaced, value to use in examples, word)
    "fillnan": ({"nan"}, "-1.0", "NaN"),
    "fillinf": ({"pinf", "ninf"}, "999.0", "infinity"),
    "fillposinf": ({"pinf"}, "1000.0", "+inf"),
    "fillneginf": ({"ninf"}, "-1000.0", "-inf"),
    "fillinvalid": ({"nan", "pinf", "ninf", "null"}, "0.0", "invalid"),
}
FLAG_NAMES = list(FLAG)
DROP_NAMES = ["dropnan", "dropinf", "dropposinf", "dropneginf", "dropinvalid"]
FILL_NAMES = ["fillnan", "fillinf", "fillposinf", "fillneginf", "fillinvalid"]
NULL_FAMILY = ["isnull", "notnull", "invalid", "dropnull", "fillnull"]

KIND_WORDS = {
    "isnan": "a `NaN`", "notnan": "a `NaN`", "dropnan": "a `NaN`", "fillnan": "a `NaN`",
    "isinf": "an infinity", "notinf": "an infinity", "dropinf": "an infinity", "fillinf": "an infinity",
    "isposinf": "`+inf`", "dropposinf": "`+inf`", "fillposinf": "`+inf`",
    "isneginf": "`-inf`", "dropneginf": "`-inf`", "fillneginf": "`-inf`",
}

# What a "cell" means on each type, and how the type reaches the one-dimensional rule.
DELEGATION = {
    "Shelf": "`Shelf` holds the rule: the question is asked of the shelf's numeric payload directly, and the answer is combined with the validity mask so that a missing entry never counts as a value.",
    "Stream": "`Stream` is a `Shelf` with an optional label, so it asks the question of its shelf and carries the label (and any metadata) across to the result.",
    "Slot": "`Slot` holds the rule for scribble. A slot cell that is a one-element number is treated exactly like a `Stream` record. A cell that holds a list of numbers is flagged when **any** number inside it is flagged.",
    "DataFrame": "`DataFrame` applies the one-dimensional rule of [`Shelf`](../shelf/{m}.md) to its numeric features. Features of the same numeric type are processed together in one batch, so the cost grows with the amount of data, not with the number of features.",
    "Catalog": "`Catalog` applies the [`Stream`](../stream/{m}.md) method to every stream. Streams may have different lengths; nothing is padded.",
    "Registry": "`Registry` applies the [`Slot`](../slot/{m}.md) method to every field. The fields of a registry are independent lists, so each is processed on its own and no alignment between fields is assumed.",
    "Atlas": "`Atlas` applies the [`DataFrame`](../dataframe/{m}.md) method to every frame in the stack and returns a new `Atlas` with the same depth.",
    "Archive": "`Archive` applies the [`Catalog`](../catalog/{m}.md) method to every catalog in the stack and returns a new `Archive` with the same depth.",
    "Axel": "`Axel` applies the [`Registry`](../registry/{m}.md) method to every registry in the stack and returns a new `Axel` with the same depth.",
    "Nexus": "`Nexus` applies the [`DataFrame`](../dataframe/{m}.md) method to every table in the workspace and returns a new `Nexus`; relationships and table metadata are kept.",
    "Locus": "`Locus` applies the [`Catalog`](../catalog/{m}.md) method to every catalog in the workspace and returns a new `Locus`; links and table metadata are kept.",
}
LOWER_LEVEL = {"DataFrame": "Shelf", "Catalog": "Stream", "Registry": "Slot", "Atlas": "DataFrame", "Archive": "Catalog",
               "Axel": "Registry", "Nexus": "DataFrame", "Locus": "Catalog"}

# What comes back, per type, for the flag methods.
FLAG_RETURNS = {
    "Shelf": "`Shelf`\n    A Bool shelf with one entry per entry of the input. Display labels are kept. It has no validity mask: a `NULL` entry is an ordinary `false` (or `true` for the `not...` methods), never `NULL`.",
    "Stream": "`Stream`\n    A Bool stream with one record per record of the input; the label is kept. It never has a validity mask.",
    "Slot": "`Slot`\n    A slot with one one-element Bool cell per input cell. An empty (`NULL`) cell gives `false` (or `true` for the `not...` methods).",
    "DataFrame": "`DataFrame`\n    A frame of Bool features with the same cases and case labels. With `features` given, only those features appear in the result. A feature that cannot hold NaN or infinity (integers, text, times) comes back all `false` (all `true` for the `not...` methods).",
    "Catalog": "`Catalog`\n    A catalog with the same keys, each stream replaced by its Bool stream.",
    "Registry": "`Registry`\n    A registry with the same field names, each slot replaced by its Bool slot.",
    "Atlas": "`Atlas`\n    A stack of the same depth; every frame replaced by its Bool frame.",
    "Archive": "`Archive`\n    A stack of the same depth; every catalog replaced by its Bool catalog.",
    "Axel": "`Axel`\n    A stack of the same depth; every registry replaced by its Bool registry.",
    "Nexus": "`Nexus`\n    A workspace with the same table names and relationships; every table replaced by its Bool frame.",
    "Locus": "`Locus`\n    A workspace with the same catalog names and links; every catalog replaced by its Bool catalog.",
}
DROP_RETURNS = {
    "Shelf": "`Shelf`\n    A new, shorter shelf holding the entries that were kept, in their original order. A validity mask on the input is carried over to the kept entries.",
    "Stream": "`Stream`\n    A new, shorter stream; the label is filtered to the same records.",
    "Slot": "`Slot`\n    A new, shorter slot holding the cells that were kept, in order.",
    "DataFrame": "`DataFrame`\n    A new frame holding the rows that were kept, with every feature present. Case labels follow their rows.",
    "Catalog": "`Catalog`\n    A new catalog in which every stream keeps the same positions; the streams stay aligned with one another.",
    "Registry": "`Registry`\n    A new registry; each field is a shorter slot. Fields may end up with different lengths.",
    "Atlas": "`Atlas`\n    A stack of the same depth. A row is removed from **every** frame when **any** frame holds the targeted kind anywhere in that row, so the frames keep the same number of rows and stay aligned row by row (an `Atlas` is a strict cube).",
    "Archive": "`Archive`\n    A stack of the same depth; each catalog is filtered on its own.",
    "Axel": "`Axel`\n    A stack of the same depth; each registry is filtered on its own.",
    "Nexus": "`Nexus`\n    A workspace with the same names and relationships; each table is filtered on its own, so tables may end up with different numbers of rows. Relationships are by key, so they stay declared.",
    "Locus": "`Locus`\n    A workspace with the same names and links; each catalog is filtered on its own.",
}
FILL_RETURNS = {
    "Shelf": "`Shelf`\n    A shelf of the same length; the matching entries hold `value`. A validity mask on the input is kept, so `NULL` entries stay `NULL`.",
    "Stream": "`Stream`\n    A stream of the same length; label and mask are kept.",
    "Slot": "`Slot`\n    A slot of the same size; matching numbers (also inside list-valued cells) hold `value`.",
    "DataFrame": "`DataFrame`\n    A frame of the same shape; the matching cells of the selected numeric features hold `value`. Other features are untouched.",
    "Catalog": "`Catalog`\n    A catalog of the same shape; every stream is filled.",
    "Registry": "`Registry`\n    A registry of the same fields; every slot is filled.",
    "Atlas": "`Atlas`\n    A stack of the same depth; every frame is filled.",
    "Archive": "`Archive`\n    A stack of the same depth; every catalog is filled.",
    "Axel": "`Axel`\n    A stack of the same depth; every registry is filled.",
    "Nexus": "`Nexus`\n    A workspace with the same names and relationships; every table is filled.",
    "Locus": "`Locus`\n    A workspace with the same names and links; every catalog is filled.",
}


def rows_table(headers: list[str], rows: list[list[str]]) -> list[str]:
    out = ["| " + " | ".join(headers) + " |", "|" + "|".join("---" for _ in headers) + "|"]
    for r in rows:
        out.append("| " + " | ".join(r) + " |")
    return out


def tf(code: str) -> list[str]:
    return ["`true`" if c == "T" else "`false`" for c in code.split()]


def truth_table(name: str, unit: str) -> list[str]:
    if name in FLAG:
        res = tf(FLAG[name][1])
        return rows_table([f"A {unit} holding", f"`{name}()`"], [[c[0], r] for c, r in zip(CELLS, res)])
    if name in DROP:
        gone = DROP[name][0]
        return rows_table([f"A {unit} holding", f"After `{name}()`"],
                          [[c[0], "removed" if c[1] in gone else "kept"] for c in CELLS])
    if name in FILL:
        gone, val, _ = FILL[name]
        rows = []
        for label, code in CELLS:
            if code in gone:
                rows.append([label, "`value`"])
            elif code == "null":
                rows.append([label, "`NULL` (unchanged)"])
            else:
                rows.append([label, "unchanged"])
        return rows_table([f"A {unit} holding", f"After `{name}(value)`"], rows)
    raise KeyError(name)


def see_also(t: T, name: str) -> list[str]:
    links = []
    fam = {
        "isnan": ["notnan", "isinf", "isnull", "invalid", "dropnan", "fillnan"],
        "notnan": ["isnan", "notinf", "notnull", "invalid"],
        "isinf": ["notinf", "isposinf", "isneginf", "isnan", "isnull", "dropinf", "fillinf"],
        "notinf": ["isinf", "notnan", "notnull"],
        "isposinf": ["isneginf", "isinf", "dropposinf", "fillposinf"],
        "isneginf": ["isposinf", "isinf", "dropneginf", "fillneginf"],
        "dropnan": ["isnan", "fillnan", "dropnull", "dropinf", "dropinvalid"],
        "dropinf": ["isinf", "fillinf", "dropposinf", "dropneginf", "dropnan", "dropinvalid"],
        "dropposinf": ["isposinf", "fillposinf", "dropneginf", "dropinf"],
        "dropneginf": ["isneginf", "fillneginf", "dropposinf", "dropinf"],
        "dropinvalid": ["invalid", "fillinvalid", "dropnull", "dropnan", "dropinf"],
        "fillnan": ["isnan", "dropnan", "fillnull", "fillinf", "fillinvalid"],
        "fillinf": ["isinf", "dropinf", "fillposinf", "fillneginf", "fillnan", "fillinvalid"],
        "fillposinf": ["isposinf", "dropposinf", "fillneginf", "fillinf"],
        "fillneginf": ["isneginf", "dropneginf", "fillposinf", "fillinf"],
        "fillinvalid": ["invalid", "dropinvalid", "fillnull", "fillnan", "fillinf"],
        "isnull": ["notnull", "invalid", "isnan", "isinf", "dropnull", "fillnull"],
        "notnull": ["isnull", "invalid", "notnan", "notinf"],
        "invalid": ["isnull", "isnan", "isinf", "dropinvalid", "fillinvalid"],
        "dropnull": ["isnull", "fillnull", "dropnan", "dropinf", "dropinvalid"],
        "fillnull": ["isnull", "dropnull", "fillnan", "fillinf", "fillinvalid"],
    }.get(name, [])
    for other in fam:
        links.append(f"- [`{t.name}::{other}`]({other}.md)")
    low = LOWER_LEVEL.get(t.name)
    if low and name not in ("count_kinds",):
        links.append(f"- [`{low}::{name}`](../{BY_NAME[low].dirname}/{name}.md): the one-dimensional rule this type delegates to.")
    return links


# ------------------------------------------------------------------------------------------------------------------------------
# Examples
# ------------------------------------------------------------------------------------------------------------------------------

A_CODES = ["ord", "nan", "pinf", "ninf", "null", "ord"]
B_CODES = ["ord", "nan", "nan", "ord", "ord", "ord"]
SINGLE = {"Shelf", "Stream", "Slot"}
STACKS = {"Atlas", "Archive", "Axel", "Nexus", "Locus"}
TWO_D = {"DataFrame", "Catalog", "Registry", "Atlas", "Archive", "Axel", "Nexus", "Locus"}
UNIT_NAMES = {"Atlas": ("first frame", "second frame"), "Archive": ("first catalog", "second catalog"),
              "Axel": ("first registry", "second registry"), "Nexus": ("table `a`", "table `b`"),
              "Locus": ("catalog `a`", "catalog `b`")}
COLNAME = {"DataFrame": "feature", "Catalog": "stream", "Registry": "field", "Atlas": "feature", "Archive": "stream",
           "Axel": "field", "Nexus": "feature", "Locus": "stream"}


def fmt_bools(vals: list[bool]) -> str:
    return "[" + ", ".join("true" if v else "false" for v in vals) + "]"


def cell_text(code: str, row: int, col: str) -> str:
    if code == "ord":
        base = {"a": {0: "1", 5: "5"}, "b": {0: "10", 3: "40", 4: "50", 5: "60"}}[col]
        return base[row]
    return {"nan": "NaN", "pinf": "inf", "ninf": "-inf", "null": "NULL"}[code]


def codes_for(col: str) -> list[str]:
    return A_CODES if col == "a" else B_CODES


def flag_vals(name: str, codes: list[str]) -> list[bool]:
    res = FLAG[name][1].split()
    order = ["ord", "nan", "pinf", "ninf", "null"]
    return [res[order.index(c)] == "T" for c in codes]


def code_line(lines: list[str]) -> list[str]:
    return lines


def program(t: T, variant: str, ops: list[str]) -> list[str]:
    body = build(t, variant) + [""] + ops
    text = "\n".join(body)
    lines = list(t.usings) + [""]
    if re.search(r"\bnan\b|\binf\b", text):
        lines += PREAMBLE_CONSTS + [""]
    if t.name in ("Slot", "Registry", "Axel") and "column(" in text:
        lines += SLOT_HELPER + [""]
    return lines + body


def example(t: T, title: str, variant: str, ops: list[str], expl: str) -> dict:
    return {"title": title, "code": program(t, variant, ops), "expl": expl}


def second_unit_sentence(t: T, clean_result: str, drop: bool = False) -> str:
    if t.name == "Atlas" and drop:
        return (" The second frame has no flagged cell of its own, but it loses the same rows: an `Atlas` is a strict cube, so every frame must keep the same "
                "number of rows and the drop is decided across the whole stack.")
    if t.name in STACKS:
        a, b = UNIT_NAMES[t.name]
        return f" The {b} has no value other than ordinary numbers, so {clean_result}."
    return ""


def flag_examples(t: T, name: str) -> list[dict]:
    what = FLAG[name][0]
    ex: list[dict] = []
    va = flag_vals(name, A_CODES)
    vb = flag_vals(name, B_CODES)
    cols = SINGLE_TEXT(t, va, vb, name)
    ops = ["auto out = x.%s();" % name] + show(t, "out")
    ex.append(example(t, f"Mark every {t.noun} that {what}, in data that holds all four kinds of cell.", "base", ops,
                      f"Position 0 and 5 are ordinary numbers, position 1 is a `NaN`, positions 2 and 3 are `+inf` and `-inf`, and position 4 is `NULL`. "
                      f"{cols}" + second_unit_sentence(t, "every answer is `%s`" % ("false" if not flag_vals(name, ['ord'])[0] else "true"))))
    ops = (["auto missing = x.isnull();", f"auto asked = x.{name}();", "auto either = x.invalid();"]
           + show(t, "missing", "isnull") + show(t, "asked", name) + show(t, "either", "invalid"))
    ex.append(example(t, "Ask the three questions on the same data, side by side.", "base", ops,
                      "`isnull` is `true` only for the `NULL` position. "
                      f"`{name}` answers the question about {KIND_WORDS[name]} and is never `true` for `NULL`" +
                      ("" if name in ("notnan", "notinf") else "") + ". "
                      "`invalid` is `true` for all three kinds of not-an-ordinary-number at once: `NULL`, `NaN` and the infinities. "
                      "The three are different questions; none of them is a synonym for another."))
    ops = ["auto out = x.%s();" % name] + show(t, "out")
    allv = flag_vals(name, ["ord"])[0]
    ex.append(example(t, "Data that cannot hold NaN or infinity: integers give the same answer everywhere.", "ints", ops,
                      f"An integer {t.noun} is never a `NaN` and never infinite, so `{name}` is `{'true' if allv else 'false'}` for all of them. "
                      "There is no error and no conversion to a floating type: the question simply has the answer \"no\"."))
    if t.name == "Slot":
        ex.append(slot_list_example(name))
    if t.name == "DataFrame":
        ops = [f'auto out = x.{name}({{"a"}});'] + show(t, "out")
        ex.append(example(t, "Ask about one feature only.", "base", ops,
                          f"With `features` given, only those features are in the result. Here the result has the single feature `a`; `b` is not asked about."))
    return ex


def SINGLE_TEXT(t: T, va: list[bool], vb: list[bool], name: str) -> str:
    if t.name in SINGLE:
        return f"`{name}` gives {fmt_bools(va)}."
    cn = COLNAME[t.name]
    s = f"For {cn} `a` the answers are {fmt_bools(va)}; {cn} `b` has `NaN` at positions 1 and 2 only, so its answers are {fmt_bools(vb)}."
    return s


def kept_rows(gone: set[str], t: T, selected_cols=("a", "b"), all_rule=False) -> list[int]:
    """Rows kept by a row-aligned type; per-field types handle each column on its own."""
    keep = []
    for r in range(6):
        hits = [codes_for(c)[r] in gone for c in selected_cols]
        drop = all(hits) if all_rule else any(hits)
        if not drop:
            keep.append(r)
    return keep


def rows_word(rows: list[int]) -> str:
    return ("row " if len(rows) == 1 else "rows ") + row_list(rows)


def row_list(rows: list[int]) -> str:
    return ", ".join(str(r) for r in rows) if rows else "none"


def drop_result_text(t: T, name: str) -> str:
    gone = DROP[name][0]
    if t.name in SINGLE:
        kept = [r for r in range(6) if A_CODES[r] not in gone]
        removed = [r for r in range(6) if A_CODES[r] in gone]
        verb = "is" if len(removed) == 1 else "are"
        return (f"The original position{'' if len(removed) == 1 else 's'} {row_list(removed)} {verb} removed; the other positions remain, in their original order, "
                "and the result is numbered again from 0.")
    if t.name in ("Registry", "Axel"):
        ka = [r for r in range(6) if A_CODES[r] not in gone]
        kb = [r for r in range(6) if B_CODES[r] not in gone]
        return (f"Each field is processed on its own: field `a` keeps positions {row_list(ka)} and field `b` keeps positions {row_list(kb)}, "
                "so the two fields end up with different lengths. A registry's fields are independent lists, not columns of one table.")
    kept = kept_rows(gone, t)
    removed = [r for r in range(6) if r not in kept]
    return (f"A row goes when **any** of its features holds the targeted kind: {rows_word(removed)} {'is' if len(removed) == 1 else 'are'} removed and {rows_word(kept)} remain, "
            "in their original order.")


def drop_examples(t: T, name: str) -> list[dict]:
    gone, word = DROP[name]
    ex: list[dict] = []
    ops = [f"auto out = x.{name}();"] + show(t, "out")
    ex.append(example(t, f"Remove the {t.plural} that hold {word if name != 'dropinvalid' else 'a NULL, a NaN or an infinity'}.", "base", ops,
                      drop_result_text(t, name) + second_unit_sentence(t, "nothing is removed", True)))
    if name == "dropinvalid":
        ops = (["auto chained = x.dropnull().dropnan().dropinf();", f"auto at_once = x.{name}();"]
               + show(t, "chained", "dropnull().dropnan().dropinf()") + show(t, "at_once", name))
        ex.append(example(t, "One call instead of three: the result is the same as chaining the three single-kind drops.", "base", ops,
                          "`dropinvalid()` is the catch-all. Chaining the three single-kind drops removes the same cells, but it makes three passes over the data and "
                          "has to be written out each time; the printed results above are identical."))
    else:
        ops = (["auto without_missing = x.dropnull();", f"auto without_this = x.{name}();"]
               + show(t, "without_missing", "dropnull") + show(t, "without_this", name))
        ex.append(example(t, f"`dropnull` and `{name}` remove different things.", "base", ops,
                          f"`dropnull()` removes only the `NULL` position (4) and leaves the `NaN` and the infinities where they are. "
                          f"`{name}()` removes only the {word} positions and leaves the `NULL` in place. Neither one is a replacement for the other."))
    ops = [f"auto out = x.{name}();"] + show(t, "out")
    ex.append(example(t, "Data with nothing to remove comes back unchanged.", "clean", ops,
                      "Every value is an ordinary number, so there is nothing to drop and every row is still there."))
    if t.name == "Slot":
        ex.append(slot_list_example(name))
    if t.name == "DataFrame":
        ops = [f'auto out = x.{name}({{"a"}});'] + show(t, "out")
        gone_rows = [r for r in range(6) if A_CODES[r] in gone]
        ex.append(example(t, "Decide by one feature only.", "base", ops,
                          f"Only feature `a` is examined, so only {rows_word(gone_rows)} {'goes' if len(gone_rows) == 1 else 'go'}. Whatever `b` holds does not matter here (its `NaN` in row 2 stays), and `b` is still in the result."))
        ops = [f'auto out = x.{name}({{"a", "b"}}, true);'] + show(t, "out")
        kept = kept_rows(gone, t, ("a", "b"), True)
        removed = [r for r in range(6) if r not in kept]
        ex.append(example(t, "`all = true`: drop a row only when every selected feature holds the targeted kind.", "base", ops,
                          f"With `all = true` a row goes only when **both** `a` and `b` hold it. That is true for {rows_word(removed)} only, so {rows_word(kept)} remain."))
    return ex


def fill_result_text(t: T, name: str, val: str) -> str:
    gone, _, word = FILL[name]
    def col_text(col: str) -> str:
        out = []
        for r, c in enumerate(codes_for(col)):
            if c in gone:
                out.append(val.rstrip("0").rstrip(".") if val.endswith(".0") else val)
            else:
                out.append(cell_text(c, r, col))
        return "[" + ", ".join(out) + "]"
    if t.name in SINGLE:
        return f"Every matching position now holds `{val}`; the result is {col_text('a')}."
    cn = COLNAME[t.name]
    return f"Every matching cell now holds `{val}`: {cn} `a` is {col_text('a')} and {cn} `b` is {col_text('b')}."


def fill_examples(t: T, name: str) -> list[dict]:
    gone, val, word = FILL[name]
    ex: list[dict] = []
    ops = [f"auto out = x.{name}({val});"] + show(t, "out")
    ex.append(example(t, f"Replace every {word if name != 'fillinvalid' else 'NULL, NaN and infinity'} with `{val}`.", "base", ops,
                      fill_result_text(t, name, val) + " " +
                      ("The `NULL` is filled too, because `fillinvalid` covers all three kinds." if name == "fillinvalid"
                       else "The `NULL` position is not touched: it is still `NULL`.") + second_unit_sentence(t, "nothing changes")))
    if name == "fillinvalid":
        chain = "auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);"
        ops = [chain] + show(t, "out")
        ex.append(example(t, "A different value for each kind: chain the single-kind fills instead.", "base", ops,
                          "`fillinvalid(value)` puts the **same** value everywhere, which is rarely what an analysis wants: a missing reading, an invalid computation and an unbounded one mean different things. "
                          "Chaining `fillnull`, `fillnan`, `fillposinf` and `fillneginf` lets each kind get its own value: here `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100."))
        ops = ["auto out = x.fillinvalid(0.0);"] + show(t, "out")
        ex.append(example(t, "Integer data holds nothing to fill (and has no NULL here), so it is unchanged.", "ints", ops,
                          "An integer cell cannot be `NaN` or infinite, and this data has no missing entries, so the call changes nothing."))
    else:
        ops = [f"auto out = x.{name}({val});", "auto still_null = out.isnull();"] + show(t, "out") + show(t, "still_null", "isnull")
        ex.append(example(t, "A fill never invents a value for a missing cell.", "base", ops,
                          f"`{name}` only touches {word} cells. The `isnull` check on the result still reports the `NULL` at position 4 (`true` there and `false` elsewhere); "
                          "filling `NULL` is the job of `fillnull`."))
        chain = "auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);"
        ops = [chain] + show(t, "out")
        ex.append(example(t, "Give each kind its own value by chaining the fills.", "base", ops,
                          f"`{name}` is one link of this chain. Chained, `NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100, so every cell ends up an ordinary number with a value chosen for what it was."))
    if t.name == "Slot":
        ex.append(slot_list_example(name))
    if t.name == "DataFrame" and name != "fillinvalid":
        ops = [f'auto out = x.{name}({val}, {{"a"}});'] + show(t, "out")
        ex.append(example(t, "Fill one feature only.", "base", ops,
                          "With `features` given, only those features are filled. The `NaN` cells of `b` stay `NaN`."))
    if t.name == "DataFrame" and name == "fillinvalid":
        ops = [f'auto out = x.fillinvalid(0.0, {{"a"}});'] + show(t, "out")
        ex.append(example(t, "Fill one feature only.", "base", ops,
                          "With `features` given, only those features are filled. The `NaN` cells of `b` stay `NaN`."))
    return ex


# ---- NULL family (Atlas, Nexus, Locus only: the other types already have these pages) -------------------------------------------

FLAG.update({
    "isnull": ("is missing (`NULL`)", "F F F F T"),
    "notnull": ("is **not** missing", "T T T T F"),
    "invalid": ("is not an ordinary number: `NULL`, `NaN` or an infinity", "F T T T T"),
})
DROP["dropnull"] = ({"null"}, "NULL")
KIND_WORDS.update({"isnull": "a missing cell", "notnull": "a missing cell", "invalid": "any of the three", "dropnull": "a missing cell", "fillnull": "a missing cell"})


def null_flag_examples(t: T, name: str) -> list[dict]:
    ex = flag_examples(t, name)
    # replace the generic "three questions" example and the integer sentence with ones that read correctly for the NULL family
    ops = (["auto missing = x.isnull();", "auto nans = x.isnan();", "auto infinite = x.isinf();", "auto either = x.invalid();"]
           + show(t, "missing", "isnull") + show(t, "nans", "isnan") + show(t, "infinite", "isinf") + show(t, "either", "invalid"))
    ex[1] = example(t, "The four questions side by side, on the same data.", "base", ops,
                    "`isnull` is `true` only at the `NULL` position (4). `isnan` is `true` only at the `NaN`, and `isinf` only at the two infinities; none of them is ever `true` for `NULL`. "
                    "`invalid` is the union: `true` for `NULL`, `NaN` and both infinities, `false` only for the ordinary numbers at positions 0 and 5.")
    allv = flag_vals(name, ["ord"])[0]
    ex[2]["expl"] = (f"Integer data here has no validity mask, so no cell is missing and none can be `NaN` or infinite: `{name}` is `{'true' if allv else 'false'}` everywhere.")
    return ex


def dropnull_examples(t: T) -> list[dict]:
    ex: list[dict] = []
    ops = ["auto out = x.dropnull();"] + show(t, "out")
    ex.append(example(t, f"Remove the {t.plural} that are missing.", "base", ops,
                      "Only the `NULL` position (4) is removed. The `NaN` and the two infinities are values, so they stay." +
                      second_unit_sentence(t, "nothing is removed", True)))
    ops = ["auto out = x.dropnull().dropnan().dropinf();"] + show(t, "out")
    ex.append(example(t, "Drop the missing cells and then the not-ordinary values: chain the three single-kind drops.", "base", ops,
                      "`dropnull` takes out `NULL`, `dropnan` takes out `NaN` and `dropinf` takes out the infinities, leaving only ordinary numbers. `dropinvalid()` does the same in one call."))
    ops = ["auto out = x.dropnull();"] + show(t, "out")
    ex.append(example(t, "Data with no missing cells comes back unchanged.", "clean", ops,
                      "There is no `NULL` anywhere, so nothing is dropped."))
    return ex


def typed_fill_fixture(t: T, kind: str) -> tuple[list[str], list[str], str]:
    """(setup lines for `frame`/`cat`, usings, fill-value expression) for a string / Timeline / Duration overload example."""
    mask = "pluss::make_series(std::vector<bool>{true, false, true})"
    if kind == "string":
        col = f'Shelf(pluss::make_note({{"Accra", "", "Tema"}}), {mask})'
        val = '"Unknown"'
        name = "city"
    elif kind == "timeline":
        col = ('Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}), ' + mask + ')')
        val = 'pluss::make_timeline({"2000-01-01T00:00:00Z"})'
        name = "opened"
    else:
        col = f"Shelf(pluss::duration::nanos({{1'000'000'000LL, 2'000'000'000LL, 3'000'000'000LL}}), {mask})"
        val = "pluss::duration::nanos(9)"
        name = "elapsed"
    return [f'auto frame = DataFrame({{{{"{name}", {col}}}}});'], name, val


def typed_fill_example(t: T, kind: str) -> dict:
    setup, col, val = typed_fill_fixture(t, kind)
    usings = list(t.usings)
    if t.name == "Atlas":
        build_ = setup + ["auto x = Atlas(std::vector<DataFrame>{frame, frame});"]
        out = ["auto out = x.fillnull(%s);" % val, 'std::cout << out << "\\n";']
    elif t.name == "Nexus":
        build_ = setup + ['auto x = Nexus({{"a", frame}, {"b", frame}});']
        out = ["auto out = x.fillnull(%s);" % val, 'std::cout << out.frame("a") << "\\n" << out.frame("b") << "\\n";']
    else:
        usings = ["using namespace pluss::bundle;", "using namespace pluss::kindle;", "using namespace pluss::table;"]
        build_ = setup + ["auto cat = frame.kindelize();", 'auto x = Locus({{"a", cat}, {"b", cat}});']
        out = ["auto out = x.fillnull(%s);" % val, f'std::cout << out.catalog("a").keys("{col}") << "\\n" << out.catalog("b").keys("{col}") << "\\n";']
    word = {"string": "text", "timeline": "timestamp", "duration": "duration"}[kind]
    title = {"string": 'Fill missing text with a string (`std::string_view` / `const char*`).',
             "timeline": "Fill missing timestamps with a `Timeline` value.",
             "duration": "Fill missing durations with a `Duration` value."}[kind]
    expl = {"string": "The empty entry at position 1 is marked missing, and the string fills it. A string only ever fills text features; numeric features are not touched by it.",
            "timeline": "Position 1 is marked missing and takes the scalar `Timeline` value. A `Timeline` only fills timestamp features.",
            "duration": "Position 1 is marked missing and takes the scalar `Duration` value. A `Duration` only fills duration features."}[kind]
    return {"title": title, "code": usings + [""] + build_ + [""] + out, "expl": expl}


def fillnull_examples(t: T) -> list[dict]:
    ex: list[dict] = []
    ops = ["auto out = x.fillnull(0.0);"] + show(t, "out")
    ex.append(example(t, "Fill every missing number with `0.0`.", "base", ops,
                      "Only the `NULL` at position 4 of `a` becomes `0`. The `NaN` and the infinities are values, not missing cells, so `fillnull` leaves them alone." +
                      second_unit_sentence(t, "nothing changes")))
    ops = ["auto out = x.fillnull(0.0);", "auto nans = out.isnan();"] + show(t, "out") + show(t, "nans", "isnan")
    ex.append(example(t, "`fillnull` never fills a NaN: the NaN positions are still NaN afterwards.", "base", ops,
                      "After `fillnull(0.0)` the `isnan` check still finds the same `NaN` cells. Filling `NaN` is the job of `fillnan`."))
    ops = ["auto out = x.fillnull(0.0).fillnan(-1.0).fillposinf(100.0).fillneginf(-100.0);"] + show(t, "out")
    ex.append(example(t, "Give each kind its own value by chaining the fills.", "base", ops,
                      "`NULL` becomes 0, `NaN` becomes -1, `+inf` becomes 100 and `-inf` becomes -100: every cell ends up an ordinary number with a value chosen for what it was."))
    for kind in ("string", "timeline", "duration"):
        ex.append(typed_fill_example(t, kind))
    return ex


# ---- counting -----------------------------------------------------------------------------------------------------------------

COUNT_FIELDS = "`null`, `nan`, `posinf`, `neginf`, `finite` and `total`"


def count_kinds_examples(t: T) -> list[dict]:
    ex: list[dict] = []
    ops = ["auto report = x.count_kinds();", "std::cout << report;"]
    cn = COLNAME[t.name]
    ex.append(example(t, f"Count the cells of each kind in every {cn}.", "base", ops,
                      f"One row per {cn}. `a` has one cell of each kind (`NULL`, `NaN`, `+inf`, `-inf`) and two ordinary numbers; `b` has two `NaN` cells and four ordinary numbers. "
                      "The kinds never overlap, so `null + nan + posinf + neginf + finite` is always `total`."))
    ops = ["auto report = x.count_kinds();", 'std::cout << report.features("nan");', 'std::cout << report.features("null");']
    ex.append(example(t, "Read one kind across all of them: which of my columns contain NaN?", "base", ops,
                      "Each column of the report is itself a column you can print, filter or compare. This is the quick audit: a non-zero `nan` or `null` count tells you which "
                      f"{cn}s need attention and what kind of attention they need."))
    ops = ["auto report = x.count_kinds();", "std::cout << report;"]
    ex.append(example(t, f"A {cn} that cannot hold NaN or infinity reports zero of each.", "ints", ops,
                      f"Integer {cn}s cannot hold `NaN` or an infinity and this data has no missing entries, so everything is `finite` and the other counts are `0`."))
    return ex


def nshelf_examples(t: T, name: str) -> list[dict]:
    word = {"nnull": "missing entries (`NULL`)", "nnan": "`NaN` entries", "nposinf": "`+inf` entries", "nneginf": "`-inf` entries"}[name]
    expected = {"nnull": 1, "nnan": 1, "nposinf": 1, "nneginf": 1}[name]
    ex = []
    ex.append(example(t, f"Count the {word}.", "base",
                      [f"auto n = x.{name}();", f'std::cout << "{name}: " << n << "\\n";'],
                      f"The data has exactly one of each kind, so `{name}()` is `{expected}`."))
    ex.append(example(t, "The four counts never double-count a cell.", "base",
                      ["auto total = x.nnull() + x.nnan() + x.nposinf() + x.nneginf();",
                       'std::cout << "not ordinary: " << total << " of " << x.size() << "\\n";',
                       'std::cout << "ordinary: " << x.size() - total << "\\n";'],
                      "`NULL`, `NaN`, `+inf` and `-inf` are four different kinds of cell, so adding the four counts gives the number of cells that are **not** ordinary numbers. "
                      "Here that is 4 of 6, leaving the 2 ordinary values."))
    ex.append(example(t, "A shelf that cannot hold NaN or infinity reports `0` for them.", "ints",
                      [f"auto n = x.{name}();", f'std::cout << "{name}: " << n << "\\n";'],
                      f"Integers can be neither `NaN` nor infinite, and this shelf has no validity mask, so `{name}()` is `0`."))
    return ex


# ------------------------------------------------------------------------------------------------------------------------------
# Page assembly
# ------------------------------------------------------------------------------------------------------------------------------

KINDS_TABLE = rows_table(
    ["A cell can be", "It means", "Ask about it with"],
    [["`NULL`", "no value was recorded (the validity mask says so)", "`isnull`, `notnull`, `dropnull`, `fillnull`"],
     ["`NaN`", "a value that is not a number: `0/0`, `x % 0`, `sqrt(-1)`", "`isnan`, `notnan`, `dropnan`, `fillnan`"],
     ["`+inf` / `-inf`", "a value that is unbounded: `5/0`, `log(0)`, overflow",
      "`isinf`, `notinf`, `isposinf`, `isneginf`, `dropinf`, `dropposinf`, `dropneginf`, `fillinf`, `fillposinf`, `fillneginf`"],
     ["any of the three", "not an ordinary number", "`invalid`, `dropinvalid`, `fillinvalid`"]])

DISPLAY_NOTE = ("Printing keeps the kinds apart: `head()`, `tail()` and `std::cout` show `NULL`, `NaN`, `inf` and `-inf`, and a PlussView window shows "
                "`NULL` on a red background and `NAN`, `INF` and `-INF` on a neutral one.")
NAT_NOTE = "A `Timeline`'s \"not a time\" (`NaT`) is a separate thing from `NaN`: it is asked about with `isnat`, never with this method."


def type_notes(t: T, name: str) -> list[str]:
    out: list[str] = []
    n = t.name
    if n == "DataFrame":
        out.append("`features` accepts a brace list of names (`{\"a\", \"b\"}`), a `std::vector<std::string>` or any other range of names. An unknown name throws a `pluss::Error`.")
    if n == "Slot":
        out.append("A cell that holds a list of numbers is one cell: it is flagged, dropped or filled according to what is **inside** it. A cell that holds text, a timestamp or a duration cannot hold `NaN` or an infinity and is never flagged.")
    if n == "Registry":
        out.append("A registry's fields are independent lists (a `Registry` is not a table), so nothing is aligned between them: each field is processed on its own and may end up a different length.")
    if n == "Catalog":
        out.append("A catalog's streams may differ in length. For `drop...` the decision is made per position: a position is removed from **every** stream when **any** stream holds the targeted kind there, which keeps the streams aligned with one another.")
    if n == "Atlas" and name in DROP:
        out.append("An `Atlas` requires every frame to have the same number of rows, so a drop cannot shorten one frame on its own. Row `r` goes from all frames when any frame has a flagged cell in row `r`; the frames stay aligned.")
    elif n in STACKS:
        a, b = UNIT_NAMES[n]
        out.append(f"Each member of the stack is processed independently, so one member's answer never depends on another's. In the examples the {a} is built from the six-row data and the {b} from clean data, which is why the second printed result differs.")
    return out


def sigs_text(overloads: list[str]) -> list[str]:
    return overloads


def what_text(t: T, name: str) -> list[str]:
    m = name
    low = LOWER_LEVEL.get(t.name)
    deleg = DELEGATION[t.name].format(m=m)
    lines: list[str] = []
    if name in FLAG:
        what = FLAG[name][0]
        lines += [f"Returns a Bool result of the same shape as the input that is `true` wherever a cell {what}. The answer is about the **value** in the cell: a missing cell has no value, so it is never a `NaN` and never infinite.",
                  "", "Three different things can sit in a numeric cell, and PlussMic keeps them apart on purpose:", ""]
        lines += KINDS_TABLE
        lines += ["", f"`{name}()` answers one precise question. Here is what it says for each kind of cell:", ""]
        lines += truth_table(name, "cell")
    elif name in DROP:
        gone, word = DROP[name]
        what = "that is `NULL`, `NaN` or an infinity" if name == "dropinvalid" else ("that is `NULL`" if name == "dropnull" else f"that holds {KIND_WORDS[name]}")
        lines += [f"Removes every {t.noun} {what} and returns the rest, in their original order. Nothing is repaired or replaced: the offending {t.plural} are simply taken out.",
                  "", f"The cells of the other kinds are left exactly as they were. Here is what `{name}()` does to each kind of cell:", ""]
        lines += truth_table(name, "cell")
        if name != "dropinvalid" and name != "dropnull":
            lines += ["", "It is the exact counterpart of the matching `is...` method: `x.%s()` keeps the positions where `x.%s()` is `false`." % (name, name.replace("drop", "is"))]
    elif name in FILL:
        gone, val, word = FILL[name]
        what = "`NULL`, `NaN` and infinity cell" if name == "fillinvalid" else f"{word} cell"
        lines += [f"Returns the same structure with every {what} replaced by `value`. The shape does not change: nothing is removed, only replaced.",
                  "", f"Here is what `{name}(value)` does to each kind of cell:", ""]
        lines += truth_table(name, "cell")
        if name == "fillinvalid":
            lines += ["", "`fillinvalid` is the **all-in-one** filler: it puts the same value everywhere. If a missing reading, an invalid computation and an unbounded one should become different values, chain the single-kind fills instead: `x.fillnull(a).fillnan(b).fillposinf(c).fillneginf(d)`."]
        else:
            lines += ["", f"A missing cell is never filled by this method, because a missing cell holds no {word} to replace; use `fillnull` for that."]
    elif name == "fillnull":
        lines += ["Replaces every missing (`NULL`) cell and leaves everything else alone. The value you pass decides which features are filled: a number fills numeric features, a string fills text features, a `Timeline` fills timestamp features and a `Duration` fills duration features.",
                  "", "A `NaN` or an infinity is a value, not a missing cell, so `fillnull` does not touch it; use `fillnan`, `fillposinf`, `fillneginf` or `fillinf` for those."]
    if t.name == "Atlas" and (name in DROP):
        deleg = ("`Atlas` applies the one-dimensional rule through [`DataFrame`](../dataframe/{m}.md), but a drop has to keep the stack a strict cube (every frame the same height). "
                 "So the decision is made across the whole stack: a row is removed from **every** frame when **any** frame holds the targeted kind anywhere in that row.").format(m=m)
    lines += ["", deleg]
    if low and name in FLAG or (low and name in DROP) or (low and name in FILL) or (low and name == "fillnull"):
        pass
    return lines


def params_text(t: T, name: str) -> list[str]:
    out: list[str] = []
    if name in FILL or name == "fillnull":
        vt = "double" if name != "fillnull" else "`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` / `const pluss::Duration&`"
        if name == "fillnull":
            out += [f"**value** ({vt})", "", "    The replacement. Its C++ type selects which features are filled. A `Timeline` or `Duration` value must be a single value.", ""]
        else:
            out += ["**value** (`double`)", "", "    The number written into every matching cell.", ""]
    if t.name == "DataFrame" and (name in FLAG or name in DROP or name in FILL):
        out += ["**features** (range of names, optional)", "",
                "    The features to look at. Omitted or empty means every feature."
                + (" Only these features appear in the result." if name in FLAG else (" Other features are left as they are." if name in FILL else " Only these features decide which rows are removed.")), ""]
        if name in DROP:
            out += ["**all** (`bool`, default `false`)", "",
                    "    `false`: a row is removed when **any** selected feature holds the targeted kind. `true`: a row is removed only when **every** selected feature does (a selected feature that cannot hold the kind never matches).", ""]
    if not out:
        out = ["`None`", ""]
    return out


def returns_text(t: T, name: str) -> str:
    if name in FLAG:
        return FLAG_RETURNS[t.name]
    if name in DROP:
        return DROP_RETURNS[t.name]
    return FILL_RETURNS[t.name]


def notes_for(t: T, name: str) -> list[str]:
    notes: list[str] = []
    if name in FLAG:
        notes.append("`NULL` is **not** `NaN`. A missing cell has no value, so `isnan`/`isinf`/`isposinf`/`isneginf` are `false` for it and the `not...` forms are `true`. Ask `isnull` for missing cells and `invalid` for \"missing or not an ordinary number\".")
    if name in ("dropinvalid", "fillinvalid"):
        notes.append("This is the catch-all for the three kinds: it also handles `NULL`. To treat the kinds differently, use the single-kind methods (`dropnull`, `dropnan`, `dropinf`, `fillnull`, `fillnan`, `fillinf`) one after another.")
    elif name in DROP or name in FILL:
        notes.append("`NULL` is **not** `NaN`: this method does not look at the validity mask. `dropnull` / `fillnull` handle missing cells; `dropinvalid` / `fillinvalid` handle all three kinds at once.")
    notes.append("Data whose type cannot hold `NaN` or an infinity (integers, booleans, text, timestamps, durations) is never affected: it has none to find, so it is reported as having none, kept whole, and left unchanged.")
    if name in FLAG or name in DROP:
        notes.append("Statistics skip `NULL` cells and **do not** skip `NaN`: a `NaN` makes `sum()`, `mean()`, `min()`, `max()`, `median()` and `std()` of its column `NaN`, and `count()` counts it as a value. An infinity propagates the way arithmetic says it should (`sum` of a column holding `+inf` is `+inf`). Use `dropnan()` (or `dropinvalid()`) first when a statistic should ignore them.")
    notes += type_notes(t, name)
    notes.append(NAT_NOTE)
    notes.append(DISPLAY_NOTE)
    return notes


def build_examples(t: T, name: str) -> list[dict]:
    if name in FLAG and name in FLAG_NAMES:
        return flag_examples(t, name)
    if name in ("isnull", "notnull", "invalid"):
        return null_flag_examples(t, name)
    if name == "dropnull":
        return dropnull_examples(t)
    if name in DROP:
        return drop_examples(t, name)
    if name == "fillnull":
        return fillnull_examples(t)
    if name in FILL:
        return fill_examples(t, name)
    if name == "count_kinds":
        return count_kinds_examples(t)
    if name in ("nnull", "nnan", "nposinf", "nneginf"):
        return nshelf_examples(t, name)
    raise KeyError(name)


def page_spec(t: T, name: str) -> dict:
    if name == "count_kinds":
        what = [f"Returns a `DataFrame` with one row per {COLNAME[t.name]} and the columns {COUNT_FIELDS}: how many of its cells are missing, `NaN`, `+inf`, `-inf`, ordinary, and how many there are in all.",
                "", "The four non-ordinary kinds never overlap, so `null + nan + posinf + neginf + finite == total` on every row. It is the quick way to audit data: one call tells you not only whether a column has bad cells but what kind they are, which decides the remedy:",
                "", *rows_table(["Count is non-zero", "Usually means", "Remedy"],
                                [["`null`", "values were never recorded", "`fillnull`, `dropnull`"],
                                 ["`nan`", "an invalid computation (`0/0`, `x % 0`, `sqrt(-1)`)", "find the cause, then `fillnan` or `dropnan`"],
                                 ["`posinf` / `neginf`", "an unbounded one (`5/0`, `log(0)`)", "`fillposinf` / `fillneginf`, or `dropinf`"]])]
        if t.name == "Catalog":
            what += ["", "Streams of different lengths are counted on their own, so the report is a normal rectangular table even when the catalog is ragged."]
        params = ["`None`", ""]
        returns = "`DataFrame`\n    One row per %s (labelled with its name); the integer columns %s." % (COLNAME[t.name], COUNT_FIELDS)
        notes = ["The report is a `DataFrame`, so everything you can do with one (`print`, `features(\"nan\")`, filtering, sorting) works on it directly.",
                 "A %s that cannot hold `NaN` or an infinity reports `0` for `nan`, `posinf` and `neginf`; its `null` count is its number of missing cells." % COLNAME[t.name],
                 DISPLAY_NOTE]
    elif name in ("nnull", "nnan", "nposinf", "nneginf"):
        word = {"nnull": "missing (`NULL`) entries", "nnan": "`NaN` entries", "nposinf": "`+inf` entries", "nneginf": "`-inf` entries"}[name]
        what = [f"Returns how many entries of the shelf are {word}, as an `int64`.", "",
                "It is the counting counterpart of the matching `is...` question: `x.%s()` equals the number of `true` in `x.%s()`." % (name, {"nnull": "isnull", "nnan": "isnan", "nposinf": "isposinf", "nneginf": "isneginf"}[name]),
                "The four counts `nnull`, `nnan`, `nposinf` and `nneginf` never overlap: a cell is exactly one of missing, `NaN`, `+inf`, `-inf` or ordinary."]
        params = ["`None`", ""]
        returns = "`std::int64_t`\n    The number of entries of that kind."
        notes = ["A shelf that cannot hold `NaN` or an infinity (integers, text, timestamps) reports `0` for `nnan`, `nposinf` and `nneginf`.",
                 "For a table-wide report use [`DataFrame::count_kinds`](../dataframe/count_kinds.md)."]
    else:
        what = what_text(t, name)
        params = params_text(t, name)
        returns = returns_text(t, name) if name != "fillnull" else FILL_RETURNS[t.name].replace("matching", "missing")
        notes = notes_for(t, name)
    return {"what": what, "params": params, "returns": returns, "notes": notes,
            "examples": build_examples(t, name), "see_also": see_also(t, name) if name not in ("count_kinds", "nnull", "nnan", "nposinf", "nneginf")
            else ([f"- [`{t.name}::isnull`](isnull.md) / [`isnan`](isnan.md) / [`isinf`](isinf.md): the per-cell questions behind the counts."] if t.name == "Shelf" else
                  ["- [`isnull`](isnull.md) / [`isnan`](isnan.md) / [`isinf`](isinf.md): the per-cell questions behind the counts.",
                   "- [`invalid`](invalid.md): `true` for all three kinds at once."])}


# ---- list-valued Slot cells ---------------------------------------------------------------------------------------------------

LIST_CELLS = [
    "auto x = Slot({",
    "    pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, nan}), pluss::make_series({2.0, inf}),",
    "    pluss::make_series({-inf, 3.0}), std::monostate{}});",
]
LIST_CONTENT = [set(), {"nan"}, {"pinf"}, {"ninf"}, None]  # None: an empty (NULL) cell
LIST_TEXT = ["`{1, 2}`", "`{1, NaN}`", "`{2, +inf}`", "`{-inf, 3}`", "an empty cell (`NULL`)"]


def slot_list_example(name: str) -> dict:
    t = BY_NAME["Slot"]
    usings = list(t.usings) + [""] + PREAMBLE_CONSTS + [""] + LIST_CELLS + [""]
    cells = LIST_CONTENT

    def hit(content, classes):
        return content is not None and bool(content & classes)

    if name in FLAG:
        target = {"isnan": {"nan"}, "notnan": {"nan"}, "isinf": {"pinf", "ninf"}, "notinf": {"pinf", "ninf"}, "isposinf": {"pinf"}, "isneginf": {"ninf"},
                  "isnull": set(), "notnull": set(), "invalid": {"nan", "pinf", "ninf"}}[name]
        res = []
        for c in cells:
            if name in ("isnull", "notnull"):
                v = c is None
            elif name == "invalid":
                v = c is None or hit(c, target)
            else:
                v = hit(c, target)
            if name in ("notnan", "notinf", "notnull"):
                v = not v
            res.append(v)
        ops = [f"auto out = x.{name}();", "for (std::int64_t i = 0; i < out.size(); ++i) {",
               '    std::cout << i << ": " << (std::get<pluss::Series>(out.at(i)).host<bool>()[0] ? "true" : "false") << "\\n";', "}"]
        what = FLAG[name][0]
        expl = (f"A cell that holds a list is **one** cell, so it gets one answer: `true` when the list contains something `{name}` asks about. "
                f"The cells are {', '.join(LIST_TEXT[:4])} and an empty cell, and the answers are {fmt_bools(res)}.")
        return {"title": "A cell that holds a list of numbers is flagged when the list contains one.", "code": usings + ops, "expl": expl}
    if name in DROP:
        gone = DROP[name][0]
        kept = [i for i, c in enumerate(cells) if not ((c is None and "null" in gone) or hit(c, gone))]
        ops = [f"auto out = x.{name}();", 'std::cout << "cells kept: " << out.size() << "\\n";', "for (std::int64_t i = 0; i < out.size(); ++i) {",
               '    if (std::holds_alternative<std::monostate>(out.at(i))) { std::cout << i << ": NULL\\n"; continue; }',
               '    std::cout << i << ": " << std::get<pluss::Series>(out.at(i)) << "\\n";', "}"]
        expl = (f"The five cells are {', '.join(LIST_TEXT[:4])} and an empty cell. A cell is dropped as a whole when what is inside it matches, so the cells that remain are the original cells {row_list(kept)}. "
                "Nothing inside a surviving cell is changed.")
        return {"title": "A cell that holds a list is dropped as a whole when the list contains the kind.", "code": usings + ops, "expl": expl}
    gone, val, word = FILL[name]
    ops = [f"auto out = x.{name}({val});", "for (std::int64_t i = 0; i < out.size(); ++i) {",
           '    if (std::holds_alternative<std::monostate>(out.at(i))) { std::cout << i << ": NULL\\n"; continue; }',
           '    std::cout << i << ": " << std::get<pluss::Series>(out.at(i)) << "\\n";', "}"]
    expl = (f"The fill reaches **inside** a list-valued cell: only the matching numbers are replaced by `{val}` and their neighbours are untouched. "
            + ("The empty cell is filled too (it becomes a one-number cell)." if name == "fillinvalid" else "The empty cell is still empty (`NULL`)."))
    return {"title": "A fill reaches inside a cell that holds a list.", "code": usings + ops, "expl": expl}
