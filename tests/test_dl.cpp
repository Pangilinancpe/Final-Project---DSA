#include <cassert>
#include <iostream>
#include "../downloads.h"
int main() {
    { DownloadManager m(1);
      assert(m.enqueue(1, 100)); assert(m.enqueue(2, 100)); assert(!m.enqueue(2, 100));
      std::vector<int> done;
      for (int i = 0; i < 400 && done.size() < 2; i++) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); auto f = m.takeFinished(); done.insert(done.end(), f.begin(), f.end()); }
      assert(done.size() == 2 && done[0] == 1 && done[1] == 2);
      for (int i = 0; i < 100 && !m.idle(); i++) std::this_thread::sleep_for(std::chrono::milliseconds(5));
      assert(m.idle()); }
    { DownloadManager m(5); m.enqueue(7, 100); m.enqueue(8, 100); } // destroyed mid download
    std::cout << "OK\n";
}
