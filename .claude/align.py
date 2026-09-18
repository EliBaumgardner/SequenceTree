#!/usr/bin/env python3
import re
import sys

CONTROL = {"if", "for", "while", "return", "else", "switch", "case", "do", "using", "template"}
COMPOUND_PREV = set("=!<>+-*/%&|^")


def mask_strings(line):
    out = []
    i = 0
    n = len(line)
    in_s = False
    in_c = False
    while i < n:
        ch = line[i]
        if in_s:
            out.append("\x00")
            if ch == "\\" and i + 1 < n:
                out.append("\x00")
                i += 2
                continue
            if ch == '"':
                out[-1] = '"'
                in_s = False
            i += 1
            continue
        if in_c:
            out.append("\x00")
            if ch == "\\" and i + 1 < n:
                out.append("\x00")
                i += 2
                continue
            if ch == "'":
                out[-1] = "'"
                in_c = False
            i += 1
            continue
        if ch == '"':
            in_s = True
            out.append(ch)
            i += 1
            continue
        if ch == "'":
            in_c = True
            out.append(ch)
            i += 1
            continue
        out.append(ch)
        i += 1
    return "".join(out)


def has_comment(line):
    m = mask_strings(line)
    return "//" in m or "/*" in m


def indent_of(line):
    return len(line) - len(line.lstrip(" "))


def first_word(line):
    m = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)", line)
    if m:
        return m.group(1)
    return ""


def find_assign(line):
    m = mask_strings(line)
    depth = 0
    for i, ch in enumerate(m):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == "=" and depth == 0:
            if i + 1 < len(m) and m[i + 1] == "=":
                return -1
            if i > 0 and m[i - 1] in COMPOUND_PREV:
                return -1
            return i
    return -1


def split_top_commas(text):
    m = mask_strings(text)
    parts = []
    depth = 0
    start = 0
    for i, ch in enumerate(m):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append(text[start:i])
            start = i + 1
    parts.append(text[start:])
    return parts


CALL_RE = re.compile(r"^(\s*)([A-Za-z_][A-Za-z0-9_]*(?:(?:\.|->|::)[A-Za-z_][A-Za-z0-9_]*)*)\s*\((.*)\)\s*;\s*$")


def parse_call(line):
    m = CALL_RE.match(line)
    if not m:
        return None
    masked = mask_strings(line)
    open_i = masked.index("(")
    depth = 0
    for i in range(open_i, len(masked)):
        if masked[i] == "(":
            depth += 1
        elif masked[i] == ")":
            depth -= 1
            if depth == 0:
                if masked[i + 1:].strip() != ";":
                    return None
                break
    return m.group(1), m.group(2), m.group(3)


def parse_assign(line):
    if has_comment(line) or line.rstrip().endswith("\\"):
        return None
    if not line.rstrip().endswith(";"):
        return None
    if first_word(line) in CONTROL:
        return None
    if line.lstrip().startswith("#"):
        return None
    idx = find_assign(line)
    if idx < 0:
        return None
    lhs = line[:idx].rstrip()
    rhs = line[idx + 1:].strip()
    if not lhs.strip() or not rhs:
        return None
    return lhs, rhs


DECL_NAME_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def split_decl(lhs):
    body = lhs.strip()
    if "(" in body or ")" in body or "[" in body:
        return None
    if " " not in body:
        return None
    head, _, name = body.rpartition(" ")
    if not DECL_NAME_RE.match(name):
        return None
    if not head.strip():
        return None
    return head.strip(), name


def already_aligned(lines):
    cols = []
    for l in lines:
        i = find_assign(l)
        if i < 0:
            return False
        cols.append(i)
    return len(set(cols)) == 1


def align_assign_run(lines):
    parsed = [parse_assign(l) for l in lines]
    if any(p is None for p in parsed):
        return None
    if already_aligned(lines):
        return list(lines)
    ind = indent_of(lines[0])
    decls = [split_decl(p[0]) for p in parsed]
    if all(d is not None for d in decls):
        head_w = max(len(d[0]) for d in decls)
        name_w = max(len(d[1]) for d in decls)
        target = head_w + 1 + name_w
        if max(target - (len(d[0]) + 1 + len(d[1])) for d in decls) > 12:
            return None
        out = []
        for (head, name), (lhs, rhs) in zip(decls, parsed):
            out.append(" " * ind + head.ljust(head_w) + " " + name.ljust(name_w) + " = " + rhs)
        return out
    lhs_w = max(len(p[0].strip()) for p in parsed)
    if lhs_w - min(len(p[0].strip()) for p in parsed) > 30:
        return None
    if any(split_decl(p[0]) is not None for p in parsed) and \
       lhs_w - min(len(p[0].strip()) for p in parsed) > 12:
        return None
    out = []
    for lhs, rhs in parsed:
        out.append(" " * ind + lhs.strip().ljust(lhs_w) + " = " + rhs)
    return out


def align_call_run(lines):
    parsed = [parse_call(l) for l in lines]
    if any(p is None for p in parsed):
        return None
    callees = {p[1] for p in parsed}
    if len(callees) != 1:
        return None
    arglists = [split_top_commas(p[2]) for p in parsed]
    ncols = len(arglists[0])
    if ncols < 2:
        return None
    if any(len(a) != ncols for a in arglists):
        return None
    cols = [[a[c].strip() for a in arglists] for c in range(ncols)]
    if any(any(x == "" for x in col) for col in cols):
        return None
    widths = [max(len(x) for x in col) for col in cols]
    for c in range(ncols - 1):
        if widths[c] - min(len(x) for x in cols[c]) > 24:
            return None
    ind = indent_of(lines[0])
    out = []
    for r in range(len(lines)):
        pieces = []
        for c in range(ncols):
            text = cols[c][r]
            if c < ncols - 1:
                pieces.append(text.ljust(widths[c]))
            else:
                pieces.append(text)
        out.append(" " * ind + parsed[r][1] + "(" + ", ".join(pieces) + ");")
    return out


def groupable(line):
    s = line.strip()
    if not s:
        return False
    if s in ("{", "}"):
        return False
    if s.startswith("#"):
        return False
    if has_comment(line):
        return False
    if line.rstrip().endswith("\\"):
        return False
    return True


def process(text):
    lines = text.split("\n")
    out = []
    i = 0
    n = len(lines)
    while i < n:
        if not groupable(lines[i]):
            out.append(lines[i])
            i += 1
            continue
        ind = indent_of(lines[i])
        j = i
        while j < n and groupable(lines[j]) and indent_of(lines[j]) == ind:
            j += 1
        run = lines[i:j]
        if len(run) >= 2:
            aligned = align_assign_run(run)
            if aligned is None:
                aligned = align_call_run(run)
            if aligned is not None:
                out.extend(aligned)
                i = j
                continue
            k = i
            while k < j:
                placed = False
                for end in range(j, k + 1, -1):
                    sub = lines[k:end]
                    a = align_assign_run(sub)
                    if a is None:
                        a = align_call_run(sub)
                    if a is not None:
                        out.extend(a)
                        k = end
                        placed = True
                        break
                if not placed:
                    out.append(lines[k])
                    k += 1
            i = j
            continue
        out.extend(run)
        i = j
    return "\n".join(out)


for path in sys.argv[1:]:
    with open(path) as f:
        original = f.read()
    updated = process(original)
    if updated != original:
        with open(path, "w") as f:
            f.write(updated)
        print("aligned:", path)
