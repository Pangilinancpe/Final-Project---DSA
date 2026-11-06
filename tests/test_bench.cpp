#include <cassert>
#include <iostream>
#include "../benchmark.h"
int main() {
    for (int s = 0; s < SHAPE_COUNT; s++) {
        auto rows = benchmarkAll(2000, (DataShape)s);
        for (auto& r : rows) assert(!r.skipped && r.correct);
        if (s == SHAPE_SORTED) assert(rows[ALGO_BUBBLE].stats.comparisons == 1999 && rows[ALGO_INSERTION].stats.comparisons == 1999);
    }
    auto big = benchmarkAll(40000, SHAPE_RANDOM);
    assert(big[ALGO_BUBBLE].skipped && big[ALGO_MERGE].correct && big[ALGO_QUICK].correct && big[ALGO_SHELL].correct);
    auto s = benchmarkSearch(1000000);
    assert(s.linearComparisons == 1000000 && s.binaryComparisons < 50);
    std::cout << "OK\n";
}
