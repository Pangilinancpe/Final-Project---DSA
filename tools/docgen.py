#!/usr/bin/env python3
"""Documentation generator for the Steam-style game store (CPE010 DSA final project).

What it does
  * finds every function and method in the headers and main.cpp (tools/docparse.py, standard library only)
  * keeps a comment block above every function:   What / Returns / Used by
      - "What" and "Returns" are written by hand in the source
      - "Used by" is computed here from the call graph and rewritten automatically
  * keeps a FUNCTION INDEX (name, kind, line) at the top of every file
  * writes docs/DOCUMENTATION.md: master index, per-module reference, include diagram, lesson map

Usage (run from the project folder)
  python3 tools/docgen.py                  write docs/DOCUMENTATION.md
  python3 tools/docgen.py --update-source  also rewrite the index and the blocks inside the source files
  python3 tools/docgen.py --check          change nothing, exit 1 when anything is out of date or missing

Optional: `pip install libclang` makes the "Used by" of class methods exact. Without it, methods are matched by name.
The code of the program is never touched, only comment lines are added or replaced.
"""
import argparse, glob, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import docparse  # noqa: E402

try:
    from clang import cindex
    HAVE_CLANG = True
except Exception:       # libclang is optional
    HAVE_CLANG = False

BLOCK_OPEN = re.compile(r"^//--- (.+?) -+\s*$")
BLOCK_CLOSE = re.compile(r"^//-{20,}\s*$")
INDEX_OPEN = "// FUNCTION INDEX of "
RULE = "//" + "=" * 76
FIELDS = ("What", "Returns", "Used by")
WIDTH = 100

# which lessons (HOA) each file shows, and its role. Used for the module table and the lesson map.
MODULES = {
    "main.cpp":       ("Program entry, screen clearing between menus (--plain / --color)", "1"),
    "store.h":        ("Main page: login, store table, cart, wallet, wishlist, reviews, friends, chat, Developer Studio, menus", "1, 3, 6, 13"),
    "screens.h":      ("Discover, Downloads, Bundle optimizer, Friend network and Algorithm lab screens", "3, 4, 5, 9, 10, 12, 13, 14"),
    "account.h":      ("Accounts, roles, wallet, wishlist, owned games, BST by username", "3, 6, 9"),
    "game.h":         ("Games and tags, linked list + BST by title, file save/load", "3, 6, 9"),
    "review.h":       ("Reviews and recommendation percent", "3"),
    "friends.h":      ("Friend requests and friendships (linked list)", "3"),
    "chat.h":         ("Chat file, live listener thread, unread counters", "13"),
    "cart.h":         ("Shopping cart (linked list)", "3"),
    "browse.h":       ("Filter by tags, sort keys, insertion sort of the store page", "7"),
    "sync.h":         ("Shared console mutex, file lock between terminals, atomic file replace", "13"),
    "tui.h":          ("Colors, tables, banners, menus, progress bar", "1"),
    "textutil.h":     ("UTF-8 aware length and cut", "1"),
    "algorithms.h":   ("Six sorts, linear/binary search, partition point, stopwatch", "6, 7, 8, 11"),
    "benchmark.h":    ("Benchmark of sorts and searches on generated data", "11, 14"),
    "stack.h":        ("ArrayStack and LinkedStack", "4"),
    "queue.h":        ("LinkedQueue and CircularQueue", "5"),
    "ring.h":         ("RingList, circular doubly linked list with a cursor", "3"),
    "history.h":      ("Back/Forward (stacks), recently viewed (circular queue), cart undo (stack)", "4, 5"),
    "downloads.h":    ("Download queue worked by a thread", "5, 13"),
    "genre.h":        ("Genre tree (general tree) with preorder, postorder and level order", "9"),
    "graph.h":        ("Friend graph: adjacency list and matrix, BFS, DFS, suggestions", "10"),
    "dp.h":           ("Dynamic programming: fibonacci, knapsack bundle, edit distance", "12"),
}
HOA = [
    ("1", "Functions"), ("2.1", "Arrays and pointers"), ("3", "Linked lists (single, double, circular)"),
    ("4", "Stacks"), ("5", "Queues"), ("6", "Searching"), ("7, 8", "Sorting"), ("9", "Trees"),
    ("10", "Graphs"), ("11", "Binary search timing"), ("12", "Dynamic programming"),
    ("13", "Multithreading"), ("14", "Sort and search benchmarks"),
]


def source_files():
    heads = sorted(os.path.basename(p) for p in glob.glob(os.path.join(ROOT, "*.h")))
    return heads + ["main.cpp"]


def read(name):
    with open(os.path.join(ROOT, name), encoding="utf-8") as f:
        return f.read()


# ---------------------------------------------------------------- doc blocks in the source
def parse_block(lines, func):
    """The doc block that ends on the line right above the function. Returns (first_line_index, fields) or None."""
    end = func.start - 2                      # index of the line above the function
    if end < 0 or not BLOCK_CLOSE.match(lines[end]):
        return None
    i = end - 1
    while i >= 0 and not BLOCK_OPEN.match(lines[i]):
        if not lines[i].startswith("//"):
            return None
        i -= 1
    if i < 0:
        return None
    fields, cur = {}, None
    for ln in lines[i + 1:end]:
        m = re.match(r"^// (What|Returns|Used by)\s*: ?(.*)$", ln)
        if m:
            cur = m.group(1)
            fields[cur] = m.group(2).strip()
        elif cur and ln.startswith("//   "):
            fields[cur] += " " + ln[2:].strip()
    return i, fields


def wrap(label, text):
    out, prefix = [], "// %-8s: " % label
    cont = "//" + " " * (len(prefix) - 2)
    words, line = text.split(), prefix
    first = True
    for w in words:
        if len(line) + len(w) + 1 > WIDTH and not (first and line == prefix):
            out.append(line.rstrip())
            line = cont
        line += ("" if line in (prefix, cont) else " ") + w
        first = False
    out.append(line.rstrip())
    return out


def render_block(func, what, returns, used):
    head = "//--- %s " % func.qual
    lines = [head + "-" * max(4, WIDTH - len(head))]
    lines += wrap("What", what)
    if returns:
        lines += wrap("Returns", returns)
    lines += wrap("Used by", used)
    lines.append("//" + "-" * (WIDTH - 2))
    return lines


def render_index(fname, funcs):
    rows = ["%s%s  (generated by tools/docgen.py, do not edit by hand)" % (INDEX_OPEN, fname),
            "//   line  function                                    kind"]
    for f in funcs:
        rows.append("//   %4d  %-42s  %s" % (f.line, f.qual, f.kind))
    if not funcs:
        rows.append("//   (no functions, only declarations)")
    return [RULE] + rows + [RULE]


def strip_index(lines):
    """Removes the index block and the one blank line that was added next to it."""
    for i, ln in enumerate(lines):
        if ln.startswith(INDEX_OPEN):
            a = i - 1 if i > 0 and lines[i - 1] == RULE else i
            j = i + 1
            while j < len(lines) and lines[j] != RULE:
                j += 1
            rest = lines[j + 1:]
            head = lines[:a]
            if a == 0:                       # the block was at the very top, a blank line follows it
                if rest and rest[0] == "":
                    rest = rest[1:]
            elif head and head[-1] == "":    # a blank line was added before it
                head = head[:-1]
            return head + rest
    return lines


def index_position(lines):
    """Right after the include guard `#define STEAM_..._H`, or the top of the file."""
    for i, ln in enumerate(lines[:15]):
        if ln.startswith("#define "):
            return i + 1
    return 0


# ---------------------------------------------------------------- call graph
def free_callers(funcs, texts):
    """Name based: a function calls another free function when its masked body contains `name(`
    (or the name as an argument, like saveAtomic(data, file, saveAccounts)).
    Free function names are unique in this project, crosscheck_clang.py verifies the result."""
    names = {f.name for f in funcs if f.kind == "function"}
    calls = {}
    for f in funcs:
        body = texts[f.file][1][f.body_start:f.body_end]
        found = set(re.findall(r"\b(\w+)\s*(?:<[^<>;(){}]*>)?\s*\(", body)) & names
        found |= set(re.findall(r"(?<=[(,])\s*(\w+)\s*(?=[,)])", body)) & names     # a function passed as an argument
        found.discard(f.name)                  # a function that calls itself is not listed as its own user
        calls[(f.file, f.line)] = found
    return calls


def clang_args():
    """Arguments for libclang. The pip package has no builtin headers (stddef.h ...), so the resource
    directory of an installed clang or gcc is added when one can be found."""
    import subprocess
    args = ["-std=c++17", "-x", "c++", "-I" + ROOT]
    for cmd in (["clang++", "-print-resource-dir"], ["clang", "-print-resource-dir"]):
        try:
            out = subprocess.run(cmd, capture_output=True, text=True, timeout=10).stdout.strip()
        except Exception:
            continue
        if out and os.path.isdir(os.path.join(out, "include")):
            args += ["-isystem", os.path.join(out, "include")]
            break
    return args


def clang_calls(funcs):
    """(file, line) of a callee -> set of (file, line) of the callers, for every function and method, from libclang."""
    idx = cindex.Index.create()
    where = {(f.file, f.line): f for f in funcs}
    result = {}
    entry_files = ["main.cpp"] + [os.path.join("tests", t) for t in sorted(os.listdir(os.path.join(ROOT, "tests")))
                                  if t.endswith(".cpp")] if os.path.isdir(os.path.join(ROOT, "tests")) else ["main.cpp"]
    for entry in entry_files:
        tu = idx.parse(os.path.join(ROOT, entry), args=clang_args())

        def loc(c):
            f = c.location.file
            if f is None:
                return None
            fn = os.path.relpath(os.path.abspath(f.name), ROOT)
            if os.path.dirname(fn) == "":
                return (fn, c.location.line)
            return (fn, c.location.line)         # tests/x.cpp keeps its folder

        def visit(c, caller):
            here = loc(c)
            if c.kind in (cindex.CursorKind.FUNCTION_DECL, cindex.CursorKind.CXX_METHOD, cindex.CursorKind.CONSTRUCTOR,
                          cindex.CursorKind.DESTRUCTOR, cindex.CursorKind.FUNCTION_TEMPLATE) and c.is_definition():
                caller = here if here in where else (here if here and here[0].startswith("tests") else caller)
            if c.kind in (cindex.CursorKind.CALL_EXPR, cindex.CursorKind.DECL_REF_EXPR, cindex.CursorKind.MEMBER_REF_EXPR):
                ref = c.referenced
                if ref is not None and caller is not None:
                    target = loc(ref)
                    if target in where and target != caller:
                        result.setdefault(target, set()).add(caller)
            for k in c.get_children():
                visit(k, caller)

        for c in tu.cursor.get_children():
            if c.location.file is not None:
                visit(c, None)
    return result


def compute_used_by(funcs, texts):
    """Returns {(file,line): sorted list of caller labels}."""
    where = {(f.file, f.line): f for f in funcs}
    used = {k: set() for k in where}
    calls = free_callers(funcs, texts)
    by_name = {f.name: f for f in funcs if f.kind == "function"}
    for caller_key, callees in calls.items():
        for name in callees:
            used[(by_name[name].file, by_name[name].line)].add(caller_key)
    extra = set()
    if HAVE_CLANG:
        try:
            cc = clang_calls(funcs)
            for callee, callers in cc.items():
                if where[callee].kind != "function":           # methods, constructors: libclang is exact
                    used[callee] |= {c for c in callers}
        except Exception as e:                                  # pragma: no cover
            print("libclang failed (%s), methods are matched by name" % e, file=sys.stderr)
            cc = None
    if not HAVE_CLANG or cc is None:
        for f in funcs:
            if f.kind in ("method", "constructor", "operator") and f.owner:
                pat = re.compile(r"(?:\.|->|\b)%s\s*\(" % re.escape(f.name))
                for g in funcs:
                    if g is f:
                        continue
                    body = texts[g.file][1][g.body_start:g.body_end]
                    if (g.owner == f.owner or g.owner == "") and pat.search(body):
                        used[(f.file, f.line)].add((g.file, g.line))
    labels = {}
    for key, callers in used.items():
        f = where[key]
        names = []
        for c in callers:
            g = where.get(c)
            if g is None:
                names.append("%s (%s)" % ("tests", c[0]))
            else:
                names.append(g.qual if g.file == f.file else "%s (%s)" % (g.qual, g.file))
        labels[key] = sorted(set(names), key=lambda s: (s.split(" (")[0].lower(), s))
    return labels, calls


# ---------------------------------------------------------------- main work
def analyse():
    names = source_files()
    texts, funcs = {}, []
    for n in names:
        t = read(n)
        texts[n] = (t, docparse.mask(t))
        funcs += docparse.find_functions(t, n)
    return names, texts, funcs


def default_used(f):
    if f.name == "main":
        return "the operating system (program entry point)"
    if f.kind == "destructor":
        return "runs by itself when the object goes out of scope or is deleted"
    return "(nothing yet: not called from the program)"


def build_source(name, text, funcs, used_labels):
    """Returns the new text of one file."""
    lines = strip_index(text.split("\n"))
    # the line numbers changed after stripping, so parse the stripped text again
    stripped = "\n".join(lines)
    mine = docparse.find_functions(stripped, name)
    by_key = {}
    for f in funcs:
        if f.file == name:
            by_key.setdefault(f.qual, []).append(f)
    seen = {}
    for f in mine:
        i = seen.get(f.qual, 0)
        seen[f.qual] = i + 1
        old = by_key[f.qual][i]
        f.old_line = old.line
    for f in reversed(mine):
        blk = parse_block(lines, f)
        what = returns = ""
        first = f.start - 1
        if blk:
            first, fields = blk
            what, returns = fields.get("What", ""), fields.get("Returns", "")
            del lines[first:f.start - 1]
        used = used_labels.get((name, f.old_line), [])
        used_text = ", ".join(used) if used else default_used(f)
        new = render_block(f, what or "TODO describe what it does", returns, used_text)
        lines[first:first] = new
    final = "\n".join(lines)
    placed = docparse.find_functions(final, name)
    shift = len(render_index(name, placed)) + 1       # the index (and one blank line) is inserted above every function
    for f in placed:
        f.line += shift
    idx = render_index(name, placed)
    lines = final.split("\n")
    pos = index_position(lines)
    lines[pos:pos] = [""] + idx if pos else idx + [""]
    return "\n".join(lines)


def generate(update_source):
    # iterate: inserting blocks moves line numbers, which the "Used by" lines and the index do not mention
    # (only the index does), so two passes always reach a fixed point.
    for _ in range(4):
        names, texts, funcs = analyse()
        used_labels, calls = compute_used_by(funcs, texts)
        changed = {}
        for n in names:
            new = build_source(n, texts[n][0], funcs, used_labels)
            if new != texts[n][0]:
                changed[n] = new
        if not changed:
            break
        if not update_source:
            return names, texts, funcs, used_labels, calls, changed
        for n, new in changed.items():
            with open(os.path.join(ROOT, n), "w", encoding="utf-8", newline="\n") as f:
                f.write(new)
    names, texts, funcs = analyse()
    used_labels, calls = compute_used_by(funcs, texts)
    return names, texts, funcs, used_labels, calls, {}


def doc_fields(f, texts):
    lines = texts[f.file][0].split("\n")
    blk = parse_block(lines, f)
    return blk[1] if blk else {}


def link(f):
    return "[%s:%d](../%s#L%d)" % (f.file, f.line, f.file, f.line)


def anchor(f):
    return re.sub(r"[^a-z0-9]+", "-", ("%s-%s-%d" % (f.file, f.qual, f.line)).lower()).strip("-")


def write_markdown(names, texts, funcs, used_labels, calls):
    where = {(f.file, f.line): f for f in funcs}
    out = []
    w = out.append
    w("# Documentation")
    w("")
    w("Generated by `tools/docgen.py`. **Do not edit this file by hand**: change the comment block above the function in the source and run the tool again.")
    w("")
    w("- Functions and methods documented: **%d** in **%d** files" % (len(funcs), len(names)))
    w("- Every function has a comment block in the source (`What`, `Returns`, `Used by`) and every file starts with its own function index.")
    w("- Class methods listed as `Class::method`. Constructors, destructors and operators are included.")
    w("")
    w("## Contents")
    w("")
    w("1. [Master index](#master-index)")
    w("2. [Modules and lessons](#modules-and-lessons)")
    w("3. [How the files include each other](#how-the-files-include-each-other)")
    w("4. [Menu map: what calls what](#menu-map-what-calls-what)")
    w("5. [Reference by file](#reference-by-file)")
    w("")
    w("## Master index")
    w("")
    w("All functions and methods in alphabetical order. The link opens the file at the line of the function.")
    w("")
    w("| Function | Kind | Where | One line |")
    w("| :--- | :--- | :--- | :--- |")
    for f in sorted(funcs, key=lambda x: (x.qual.lower(), x.file, x.line)):
        what = doc_fields(f, texts).get("What", "")
        short = what.split(". ")[0].rstrip(".")
        w("| [`%s`](#%s) | %s | %s | %s |" % (f.qual, anchor(f), f.kind, link(f), short.replace("|", "\\|")))
    w("")
    w("## Modules and lessons")
    w("")
    w("| File | What it contains | HOA | Functions |")
    w("| :--- | :--- | :---: | :---: |")
    for n in names:
        role, hoa = MODULES.get(n, ("", ""))
        w("| `%s` | %s | %s | %d |" % (n, role, hoa, sum(1 for f in funcs if f.file == n)))
    w("")
    w("### Lesson map")
    w("")
    w("| HOA | Lesson | Files |")
    w("| :---: | :--- | :--- |")
    for code, title in HOA:
        files = [n for n in names if n in MODULES and code.split(",")[0].strip() in [x.strip() for x in MODULES[n][1].split(",")]
                 or (n in MODULES and code == "7, 8" and ("7" in [x.strip() for x in MODULES[n][1].split(",")] or "8" in [x.strip() for x in MODULES[n][1].split(",")]))]
        w("| %s | %s | %s |" % (code, title, ", ".join("`%s`" % x for x in files) or "(shown across the program)"))
    w("")
    w("## How the files include each other")
    w("")
    w("```mermaid")
    w("graph LR")
    for n in names:
        for m in re.findall(r'^#include "([^"]+)"', texts[n][0], re.M):
            w("    %s --> %s" % (n.replace(".", "_"), m.replace(".", "_")))
    w("```")
    w("")
    w("## Menu map: what calls what")
    w("")
    w("Starting at `main`, three levels deep. A name in `code` is a function documented below.")
    w("")
    by_name = {f.name: f for f in funcs if f.kind == "function"}

    def tree(name, depth, seen):
        f = by_name.get(name)
        if f is None:
            return
        w("%s- `%s` (%s)" % ("  " * depth, name, link(f)))
        if depth >= 3 or name in seen:
            return
        for callee in sorted(calls.get((f.file, f.line), ())):
            if callee in ("readLine", "readInt", "paint", "fit", "tuiSection", "tuiRule", "gameTrim", "moneyText", "priceText"):
                continue
            tree(callee, depth + 1, seen | {name})
    for root in ("main",):
        tree(root, 0, set())
    w("")
    w("## Reference by file")
    for n in names:
        w("")
        w("### `%s`" % n)
        w("")
        role, hoa = MODULES.get(n, ("", ""))
        if role:
            w("%s. Lessons: HOA %s." % (role, hoa))
            w("")
        fs = [f for f in funcs if f.file == n]
        if not fs:
            w("_No functions in this file._")
            continue
        w("| Line | Function | Kind |")
        w("| ---: | :--- | :--- |")
        for f in fs:
            w("| %d | [`%s`](#%s) | %s |" % (f.line, f.qual, anchor(f), f.kind))
        for f in fs:
            fields = doc_fields(f, texts)
            w("")
            w('<a id="%s"></a>' % anchor(f))
            w("#### `%s`" % f.qual)
            w("")
            w("`%s`  " % f.signature.replace("`", "'"))
            w("%s, %s" % (f.kind, link(f)))
            w("")
            w("- **What:** %s" % fields.get("What", ""))
            if fields.get("Returns"):
                w("- **Returns:** %s" % fields["Returns"])
            w("- **Used by:** %s" % fields.get("Used by", ""))
            callees = sorted(calls.get((f.file, f.line), ()))
            if callees:
                w("- **Calls:** %s" % ", ".join("`%s`" % c for c in callees))
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--update-source", action="store_true", help="rewrite the comment blocks and indexes in the source files")
    ap.add_argument("--check", action="store_true", help="change nothing, fail when anything is out of date")
    args = ap.parse_args()

    names, texts, funcs, used_labels, calls, changed = generate(update_source=args.update_source and not args.check)
    if args.check:
        problems = ["%s: comment blocks or index are out of date (run: python3 tools/docgen.py --update-source)" % n for n in changed]
        for f in funcs:
            if "TODO" in doc_fields(f, texts).get("What", "TODO"):
                problems.append("%s:%d %s has no description" % (f.file, f.line, f.qual))
        md = write_markdown(names, texts, funcs, used_labels, calls)
        target = os.path.join(ROOT, "docs", "DOCUMENTATION.md")
        if not os.path.exists(target) or open(target, encoding="utf-8").read() != md:
            problems.append("docs/DOCUMENTATION.md is out of date (run: python3 tools/docgen.py)")
        for p in problems:
            print(p)
        print("OK: documentation is up to date (%d functions)" % len(funcs) if not problems else "%d problem(s)" % len(problems))
        return 1 if problems else 0

    md = write_markdown(names, texts, funcs, used_labels, calls)
    os.makedirs(os.path.join(ROOT, "docs"), exist_ok=True)
    with open(os.path.join(ROOT, "docs", "DOCUMENTATION.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write(md)
    print("%d functions in %d files -> docs/DOCUMENTATION.md%s" % (len(funcs), len(names),
          " (libclang: exact method callers)" if HAVE_CLANG else " (no libclang: methods matched by name)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
