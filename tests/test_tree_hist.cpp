#include <cassert>
#include <iostream>
#include "../genre.h"
#include "../history.h"
int main() {
    GenreNode* r = genreBuildDefault();
    assert(genreCount(r) == 19 && genreHeight(r) == 3);
    std::vector<GenreVisit> pre, post, lvl;
    genrePreorder(r, 0, pre); genrePostorder(r, 0, post); genreLevelOrder(r, lvl);
    assert(pre.size() == 19 && post.size() == 19 && lvl.size() == 19);
    assert(pre[0].node == r && post[18].node == r && lvl[0].node == r);
    assert(pre[1].node->name == "action" && pre[2].node->name == "shooter");
    assert(lvl[1].depth == 1 && lvl[7].depth == 1 && lvl[8].depth == 2);
    genreFind(r, "shooter")->gameCount = 3; genreFind(r, "action")->gameCount = 2;
    assert(genreTotal(genreFind(r, "action")) == 5);
    std::vector<std::string> path; assert(genrePath(r, "shooter", path) && path.size() == 3 && path[1] == "action");
    path.clear(); assert(!genrePath(r, "nope", path) && path.empty());
    genreFree(r);
    BrowseHistory h; h.visit(1); h.visit(2); h.visit(3);
    assert(h.goBack() == 2 && h.goBack() == 1 && h.goBack() == 0 && h.goForward() == 2);
    h.visit(9); assert(!h.canGoForward() && h.current() == 9);
    RecentlyViewed rv(3); rv.add(1); rv.add(2); rv.add(3); rv.add(2); rv.add(4);
    auto v = rv.newestFirst(); assert(v.size() == 3 && v[0] == 4 && v[1] == 2 && v[2] == 3);
    CartUndo u; u.record(5, true); u.record(6, false); CartAction a;
    assert(u.pop(a) && a.gameId == 6 && !a.wasAdd && u.size() == 1);
    std::cout << "OK\n";
}
