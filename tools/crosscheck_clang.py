"""Cross-check of the function inventory: compares docparse.py with libclang.
Needs `pip install libclang`. Usage: python3 tools/crosscheck_clang.py
Exit code 0 when both agree on every function definition (file, line of the name)."""
import glob, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import docparse
import docgen
from clang import cindex

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KINDS = {cindex.CursorKind.FUNCTION_DECL, cindex.CursorKind.CXX_METHOD, cindex.CursorKind.CONSTRUCTOR,
         cindex.CursorKind.DESTRUCTOR, cindex.CursorKind.FUNCTION_TEMPLATE, cindex.CursorKind.CONVERSION_FUNCTION}


def clang_functions(main="main.cpp"):
    idx = cindex.Index.create()
    tu = idx.parse(os.path.join(ROOT, main), args=docgen.clang_args())
    errors = [d for d in tu.diagnostics if d.severity >= cindex.Diagnostic.Error]
    if errors:
        print("libclang reported errors, the result may be incomplete:", errors[0])
    out = set()

    def walk(cur):
        for c in cur.get_children():
            f = c.location.file
            if f is None or os.path.dirname(os.path.abspath(f.name)) != ROOT:
                continue
            if c.kind in KINDS and c.is_definition():
                out.add((os.path.basename(f.name), c.location.line))
            if c.kind in (cindex.CursorKind.CLASS_DECL, cindex.CursorKind.STRUCT_DECL, cindex.CursorKind.CLASS_TEMPLATE,
                          cindex.CursorKind.NAMESPACE):
                walk(c)
    walk(tu.cursor)
    return out


def main():
    files = sorted(glob.glob(os.path.join(ROOT, "*.h"))) + [os.path.join(ROOT, "main.cpp")]
    mine = set()
    for p in files:
        for f in docparse.find_functions(open(p, encoding="utf-8").read(), os.path.basename(p)):
            mine.add((f.file, f.line))
    theirs = clang_functions()
    only_mine, only_clang = sorted(mine - theirs), sorted(theirs - mine)
    print(f"docparse: {len(mine)}   libclang: {len(theirs)}")
    for x in only_mine:
        print("  only docparse:", x)
    for x in only_clang:
        print("  only libclang:", x)
    # second check: callers of FREE functions, name based (docgen) against libclang
    names, texts, funcs = docgen.analyse()
    where = {(f.file, f.line): f for f in funcs}
    by_name = {f.name: f for f in funcs if f.kind == "function"}
    by_name_calls = docgen.free_callers(funcs, texts)
    mine_calls = set()
    for caller, callees in by_name_calls.items():
        for name in callees:
            mine_calls.add((caller, (by_name[name].file, by_name[name].line)))
    cc = docgen.clang_calls(funcs)
    clang_pairs = set()
    for callee, callers in cc.items():
        if where[callee].kind == "function":
            for caller in callers:
                if caller in where:
                    clang_pairs.add((caller, callee))
    # templates: a call that depends on a template argument can not be resolved by libclang, so only
    # calls that libclang CAN see must also be found by the name based search
    missing = sorted(p for p in clang_pairs if p not in mine_calls)
    extra = sorted(p for p in mine_calls if p not in clang_pairs)
    print("call graph (free functions): name based %d pairs, libclang %d pairs" % (len(mine_calls), len(clang_pairs)))
    for caller, callee in missing:
        print("  only libclang: %s -> %s" % (where[caller].qual, where[callee].qual))
    for caller, callee in extra:
        print("  only name based: %s -> %s" % (where[caller].qual, where[callee].qual))
    return 0 if not only_mine and not only_clang and not missing else 1


if __name__ == "__main__":
    sys.exit(main())
