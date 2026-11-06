#include <cassert>
#include <iostream>
#include <algorithm>
#include <random>
#include <string>
#include "../algorithms.h"
#include "../stack.h"
#include "../queue.h"
#include "../ring.h"
#include "../textutil.h"

struct P { int key; int id; };
int main() {
    std::mt19937 rng(7);
    for (int algo = 0; algo < ALGO_COUNT; algo++) {
        for (int n : {0, 1, 2, 3, 10, 100, 1000}) {
            for (int range : {5, 100000}) {
                std::vector<P> v(n);
                for (int i = 0; i < n; i++) v[i] = {(int)(rng() % range), i};
                auto ref = v;
                auto less = [](const P& a, const P& b) { return a.key < b.key; };
                std::stable_sort(ref.begin(), ref.end(), less);
                SortStats st;
                sortWith((SortAlgo)algo, v.data(), n, less, &st);
                assert(isSorted(v.data(), n, less));
                for (int i = 0; i < n; i++) assert(v[i].key == ref[i].key);
                if (sortAlgoIsStable((SortAlgo)algo))
                    for (int i = 0; i < n; i++) assert(v[i].id == ref[i].id);
            }
        }
        // sorted + reversed input
        std::vector<int> s(500), r(500);
        for (int i = 0; i < 500; i++) { s[i] = i; r[i] = 500 - i; }
        auto lt = [](int a, int b) { return a < b; };
        sortWith((SortAlgo)algo, s.data(), 500, lt);
        sortWith((SortAlgo)algo, r.data(), 500, lt);
        assert(isSorted(s.data(), 500, lt) && isSorted(r.data(), 500, lt));
    }
    std::vector<int> a = {1,3,3,3,5,7,9};
    auto lt = [](int x, int y) { return x < y; };
    assert(binarySearch(a.data(), 7, 7, lt) == 5);
    assert(binarySearch(a.data(), 7, 4, lt) == -1);
    assert(linearSearch(a.data(), 7, 9) == 6);
    int lo = partitionPoint(a.data(), 7, [](int x){return x < 3;});
    int hi = partitionPoint(a.data(), 7, [](int x){return x <= 3;});
    assert(lo == 1 && hi == 4);

    ArrayStack<int> as(2); LinkedStack<int> ls; int x;
    for (int i = 0; i < 20; i++) { as.push(i); ls.push(i); }
    ArrayStack<int> as2(as); LinkedStack<int> ls2; ls2 = ls;
    for (int i = 19; i >= 0; i--) { assert(as.pop(x) && x == i); assert(ls.pop(x) && x == i); assert(as2.pop(x) && x==i); assert(ls2.pop(x) && x==i);}
    assert(!as.pop(x) && !ls.peek(x));
    LinkedQueue<int> lq; CircularQueue<int> cq(4);
    for (int i = 0; i < 4; i++) { lq.enqueue(i); assert(cq.enqueue(i)); }
    assert(!cq.enqueue(9));
    cq.enqueueOverwrite(4);
    assert(cq.at(0, x) && x == 1 && cq.at(3, x) && x == 4);
    CircularQueue<int> cq2(cq); lq.enqueue(4);
    for (int i = 1; i <= 4; i++) { assert(cq2.dequeue(x) && x == i); }
    for (int i = 0; i < 5; i++) { assert(lq.dequeue(x) && x == i); }
    assert(lq.empty() && !lq.dequeue(x));
    RingList<int> rg;
    for (int i = 1; i <= 5; i++) rg.pushBack(i);
    rg.pushFront(0);
    rg.movePrev(); rg.movePrev();
    RingList<int> rg2 = rg;
    rg.current(x); assert(x == 5);
    rg2.current(x); assert(x == 5 && rg2.size() == 6 && rg2.position() == 6);
    for (int i = 0; i < 6; i++) rg.moveNext();
    rg.current(x); assert(x == 5);
    while (rg.removeCurrent()) {}
    assert(rg.empty());
    assert(utf8Length("caf\xc3\xa9") == 4 && utf8Prefix("caf\xc3\xa9", 4) == "caf\xc3\xa9" && utf8Prefix("caf\xc3\xa9", 3) == "caf");
    std::cout << "ALL OK\n";
}
