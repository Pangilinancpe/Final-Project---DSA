#include <cassert>
#include <iostream>
#include "../graph.h"
#include "../dp.h"
static void link(FriendList& l, int a, int b, bool acc = true) {
    FriendNode* n = new FriendNode; n->fromId = a; n->toId = b; n->accepted = acc;
    if (!l.head) l.head = n; else l.tail->next = n; l.tail = n; l.count++;
}
int main() {
    FriendList l;
    link(l,1,2); link(l,2,3); link(l,3,4); link(l,1,5); link(l,5,3); link(l,7,8); link(l,1,9,false);
    FriendGraph g = buildFriendGraph(l, 1);
    int v1 = graphVertexOf(g,1), v4 = graphVertexOf(g,4), v8 = graphVertexOf(g,8), v9 = graphVertexOf(g,9);
    assert(v9 == -1);
    auto d = graphBfsDistances(g, v1);
    assert(d[v4] == 3 && d[v8] == -1 && d[graphVertexOf(g,3)] == 2);
    auto p = graphShortestPath(g, v1, v4);
    assert(p.size() == 4 && p[0] == v1 && p[3] == v4);
    assert(graphShortestPath(g, v1, v8).empty());
    assert(graphDfsOrder(g, v1).size() == 5);
    assert(graphAreFriends(g, v1, graphVertexOf(g,2)) && !graphAreFriends(g, v1, v4));
    auto s = graphSuggestFriends(g, v1);
    assert(s.size() == 1 && s[0].userId == 3 && s[0].mutualFriends == 2);
    assert(fibonacciMemo(50) == 12586269025LL);
    long long c1=0,c2=0; assert(fibonacciNaive(20,&c1)==fibonacciMemo(20,&c2)); assert(c1 > c2);
    std::vector<BundleItem> it = {{1,1000,60},{2,2000,100},{3,3000,120}};
    BundleResult r = bestBundle(it, 5000);
    assert(r.totalValue == 220 && r.totalCostCents == 5000 && r.gameIds.size()==2 && r.gameIds[0]==2);
    assert(bestBundle(it, 500).gameIds.empty());
    assert(editDistance("kitten","sitting")==3 && editDistance("","abc")==3 && editDistance("caf\xc3\xa9","cafe")==1);
    freeFriends(l); std::cout << "OK\n";
}
