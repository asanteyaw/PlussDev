#!/usr/bin/env python3
"""Moves the return type of a signature in the PlussMic docs to the front: `Cls::f(args) const -> Ret` becomes
`Ret Cls::f(args) const`, the way every other Pluss reference writes a signature.

It rewrites signature lines of fenced blocks (not lambdas, not Python) and signatures inside inline code spans.
A signature that already begins with its return type (`static Ret f(args) -> Ret`) just loses the arrow.

Usage: python3 restyle_signatures.py [--check] [paths...]   (default: every .md under this directory except _build)
"""
import pathlib, re, sys

HERE = pathlib.Path(__file__).resolve().parent
QUALS = r"(?:\s*(?:const|noexcept|&&|&|volatile|override))*"
RET_CHARS = r"[\w:<>,&\*\s\.\[\]]+?"
HEAD_START = re.compile(r"^(?P<indent>\s*(?:[-*]\s+)?)(?P<static>(?:static|virtual|inline|friend)\s+)?(?P<head>[A-Za-z_][\w:<>]*(?:::[~\w]+|\.\w+)*(?:\(\))?(?:\.[\w]+)*\()")
LEADING = re.compile(r"^\s*(?:[-*]\s+)?(?:(?:static|virtual|inline)\s+)?[\w:<>&\*,]+\s+[~\w:.]+(?:\(\))?(?:\.\w+)*\(")
ARROW_TAIL = re.compile(r"\)(?P<quals>" + QUALS + r")\s*->\s*(?P<ret>" + RET_CHARS + r")\s*(?P<comment>//.*)?$")


def has_leading_type(text_before_paren):
    """`DataFrame load(` or `static DataFrame load(`: something precedes the function name."""
    t = re.sub(r"^(?:static|virtual|inline|friend)\s+", "", text_before_paren.strip())
    return bool(re.search(r"[\w>&\*]\s+[~\w:]+$", t))


def convert_signature(line, ret):
    m = HEAD_START.match(line)
    if not m:
        return None
    before = line[: line.index("(")]
    if has_leading_type(before):
        return line  # arrow only repeated the type that already leads
    return line[: m.start("static") if m.group("static") else m.start("head")] + (m.group("static") or "") + ret.strip() + " " + line[m.start("head"):]


def convert_block(lines):
    """Convert the signature statements in one fenced block (list of lines)."""
    out = list(lines)
    i = 0
    changed = 0
    while i < len(out):
        arrow = ARROW_TAIL.search(out[i])
        if not arrow:
            i += 1
            continue
        # find the first line of the statement: walk up until the parentheses balance
        balance = out[i][: arrow.start() + 1].count(")") - out[i][: arrow.start() + 1].count("(")
        start = i
        while balance > 0 and start > 0:
            start -= 1
            balance += out[start].count(")") - out[start].count("(")
        ret = arrow.group("ret")
        cut_tail = (out[i][: arrow.start() + 1] + arrow.group("quals").rstrip()).rstrip()
        comment = arrow.group("comment")
        head_line = cut_tail if start == i else out[start]
        if LEADING.match(head_line) and not HEAD_START.match(head_line):
            new_head = head_line  # the signature already begins with its return type: the arrow only repeats it
        else:
            new_head = convert_signature(head_line, ret)
        if new_head is None:
            i += 1
            continue
        if start == i:
            out[i] = new_head + (("  " + comment) if comment else "")
        else:
            out[start] = new_head
            out[i] = cut_tail + (("  " + comment) if comment else "")
        changed += 1
        i += 1
    return out, changed


INLINE = re.compile(r"`(?P<span>[^`\n]*?\)" + QUALS + r"\s*->\s*" + RET_CHARS + r")`")


def convert_inline(line):
    count = 0

    def repl(m):
        nonlocal count
        span = m.group("span")
        arrow = ARROW_TAIL.search(span)
        if not arrow:
            return m.group(0)
        head = span[: arrow.start() + 1]
        quals = arrow.group("quals").rstrip()
        ret = arrow.group("ret")
        if LEADING.match(head) and not HEAD_START.match(head):
            count += 1
            return "`" + head + quals + "`"
        if not HEAD_START.match(head):
            return m.group(0)
        if has_leading_type(head[: head.index("(")]):
            count += 1
            return "`" + head + quals + "`"
        new = convert_signature(head, ret)
        if new is None:
            return m.group(0)
        count += 1
        return "`" + new + quals + "`"
    return INLINE.sub(repl, line), count


def convert_text(text):
    lines = text.split("\n")
    out = []
    i = 0
    total = 0
    while i < len(lines):
        line = lines[i]
        fence = re.match(r"^(\s*)```(\w*)", line)
        if fence and not line.strip().startswith("```{"):
            lang = fence.group(2).lower()
            j = i + 1
            while j < len(lines) and not re.match(r"^\s*```\s*$", lines[j]):
                j += 1
            block = lines[i + 1:j]
            if lang in ("python", "py", "bash", "sh", "json"):
                new_block, c = block, 0
            else:
                new_block, c = convert_block(block)
            total += c
            out.append(line)
            out.extend(new_block)
            if j < len(lines):
                out.append(lines[j])
            i = j + 1
            continue
        new_line, c = convert_inline(line)
        total += c
        out.append(new_line)
        i += 1
    return "\n".join(out), total


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    check = "--check" in sys.argv
    paths = [pathlib.Path(a) for a in args] or [p for p in HERE.rglob("*.md") if "_build" not in p.parts and "_local" not in p.parts]
    files = changed = total = 0
    for p in sorted(paths):
        text = p.read_text()
        new, c = convert_text(text)
        files += 1
        if new != text:
            changed += 1
            total += c
            if not check:
                p.write_text(new)
    print(f"{files} files, {changed} {'to change' if check else 'changed'}, {total} signatures")


if __name__ == "__main__":
    main()
