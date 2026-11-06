"""Small C++ function finder used by docgen.py (standard library only).

It does not understand all of C++, only the style used in this project:
header-only files with free functions, classes/structs with inline methods and templates.
Function bodies are skipped, so lambdas inside a function are not listed.
"""
import re
from dataclasses import dataclass, field


@dataclass
class Func:
    name: str                 # plain name, e.g. "push"
    qual: str                 # qualified name, e.g. "ArrayStack::push" (same as name for free functions)
    kind: str                 # function | method | constructor | destructor | operator
    file: str                 # file name only, e.g. "stack.h"
    line: int                 # line of the name token (1-based), the same line libclang reports
    start: int                # first line of the definition (includes a "template <...>" line)
    body_start: int           # offset of "{" in the file text
    body_end: int             # offset just after the matching "}"
    signature: str            # one line, whitespace collapsed
    is_template: bool = False
    owner: str = ""           # class name for members
    doc: dict = field(default_factory=dict)   # filled by docgen from the doc block above


def mask(text):
    """Same length as text, but comments, strings, chars and preprocessor lines are blanked (newlines kept)."""
    out = list(text)
    i, n = 0, len(text)
    line_start = True
    while i < n:
        c = text[i]
        if line_start and c in " \t":
            j = i
            while j < n and text[j] in " \t":
                j += 1
            if j < n and text[j] == "#":
                i = j
                c = "#"
            else:
                i = j
                line_start = False
                continue
        if c == "#" and line_start:
            while i < n:
                if text[i] == "\n" and text[i - 1] != "\\":
                    break
                if text[i] != "\n":
                    out[i] = " "
                i += 1
            continue
        if c == "\n":
            line_start = True
            i += 1
            continue
        line_start = False
        if text.startswith("//", i):
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end == -1 else end + 2
            for k in range(i, end):
                if text[k] != "\n":
                    out[k] = " "
            i = end
            continue
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                if text[j] == "\\":
                    j += 1
                j += 1
            for k in range(i + 1, min(j, n)):
                if text[k] != "\n":
                    out[k] = " "
            i = j + 1
            continue
        i += 1
    return "".join(out)


def match_brace(code, open_pos):
    depth = 0
    for i in range(open_pos, len(code)):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
    return len(code)


ACCESS = re.compile(r"\b(public|private|protected)\s*:")
TYPE_HEAD = re.compile(r"\b(class|struct|union)\s+(\w+)")
NAME_RE = re.compile(r"(operator\s*(?:\(\)|\[\]|<<|>>|==|!=|<=|>=|[-+*/%&|^~!=<>]+|new|delete)|~?\w+)\s*\(")


def find_functions(text, filename):
    code = mask(text)
    line_of = _line_index(text)
    found = []
    _scan(code, text, filename, 0, len(code), "", found, line_of)
    return found


def _line_index(text):
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)

    def line_of(pos):
        lo, hi = 0, len(starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if starts[mid] <= pos:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1
    return line_of


def _scan(code, text, filename, lo, hi, owner, found, line_of):
    i = lo
    stmt = lo                       # start of the current statement
    while i < hi:
        c = code[i]
        if c == ";":
            stmt = i + 1
        elif c == "}":
            stmt = i + 1
        elif c == ":" and ACCESS.search(code[stmt:i + 1]) and not code[i + 1:i + 2] == ":":
            stmt = i + 1
        elif c == "(":                      # skip parameter lists and calls so "{" inside them is not misread
            depth = 0
            while i < hi:
                if code[i] == "(":
                    depth += 1
                elif code[i] == ")":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
        elif c == "{":
            head = code[stmt:i]
            end = match_brace(code, i)
            head_clean = re.sub(r"template\s*<[^{};]*?>\s*(?=(?:class|struct|\w))", "", head, count=1) if "template" in head else head
            m_type = TYPE_HEAD.search(head_clean)
            if re.search(r"\bnamespace\b", head_clean):
                _scan(code, text, filename, i + 1, end - 1, owner, found, line_of)
            elif re.match(r"\s*enum\b", head_clean) or re.search(r"\benum\b", head_clean) and "(" not in head_clean:
                pass
            elif m_type and "(" not in head_clean.split(m_type.group(0))[0] + head_clean[m_type.end():].split(":")[0]:
                _scan(code, text, filename, i + 1, end - 1, m_type.group(2), found, line_of)
            elif "(" in head_clean and "=" not in re.sub(r"operator\s*[^\s(]+", "", head_clean.split("(")[0]):
                f = _make_func(code, text, filename, stmt, i, end, owner, head, line_of)
                if f:
                    found.append(f)
            i = end
            stmt = end
            continue
        i += 1


def _make_func(code, text, filename, stmt, brace, end, owner, head, line_of):
    head_nt = head
    is_tpl = bool(re.match(r"\s*template\s*<", head))
    if is_tpl:                               # drop the template<...> prefix, matching angle brackets
        j = head.index("<")
        depth = 0
        for k in range(j, len(head)):
            if head[k] == "<":
                depth += 1
            elif head[k] == ">":
                depth -= 1
                if depth == 0:
                    head_nt = head[k + 1:]
                    break
    m = NAME_RE.search(head_nt)
    if not m:
        return None
    name = re.sub(r"\s+", "", m.group(1))
    if name in ("if", "for", "while", "switch", "return"):
        return None
    # real start: first non-space character of the statement
    first = stmt
    while first < brace and code[first] in " \t\r\n":
        first += 1
    name_pos = brace - len(head_nt) + m.start(1) if False else (stmt + len(head) - len(head_nt) + m.start(1))
    if owner and name == owner:
        kind = "constructor"
    elif owner and name == "~" + owner:
        kind = "destructor"
    elif name.startswith("operator"):
        kind = "operator"
    elif owner:
        kind = "method"
    else:
        kind = "function"
    sig = re.sub(r"\s+", " ", text[first:brace]).strip()
    qual = f"{owner}::{name}" if owner else name
    return Func(name=name, qual=qual, kind=kind, file=filename, line=line_of(name_pos), start=line_of(first),
                body_start=brace, body_end=end, signature=sig, is_template=is_tpl, owner=owner)
