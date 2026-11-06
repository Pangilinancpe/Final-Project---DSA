<div align="center">

<img width="360" alt="Game Store logo" src="https://github.com/user-attachments/assets/2789f12e-50c7-443f-8c3a-4bbfd37bd2aa" />

# Game Store

**A Steam-style game platform for the terminal, written in C++17**

Accounts &bull; Developer Studio &bull; Tags and sorting &bull; Cart and wallet &bull; Friends and live chat &bull; Downloads &bull; Algorithm lab

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Interface](https://img.shields.io/badge/Interface-Colored_TUI-8A2BE2?style=for-the-badge)
![Threads](https://img.shields.io/badge/Multithreaded-std%3A%3Athread-28A745?style=for-the-badge)
![Course](https://img.shields.io/badge/CPE_010-Data_Structures_%26_Algorithms-FF8C00?style=for-the-badge)

<p>
  <a href="#features">Features</a> &bull;
  <a href="#quick-start">Quick start</a> &bull;
  <a href="#how-to-use-it">How to use it</a> &bull;
  <a href="#project-structure">Structure</a> &bull;
  <a href="#lessons-applied-hoa-1-to-14">HOA coverage</a> &bull;
  <a href="#documentation">Documentation</a> &bull;
  <a href="#testing">Testing</a>
</p>

</div>

---

## About

Game Store is the **final project of CPE 010 - Data Structures and Algorithms** (Technological Institute of the Philippines). It is a mock-up of a game storefront like Steam that runs in the console. The goal of the project is to apply the lessons of **Hands-On Activities 1 to 14** inside one real program instead of separate exercises. **Every lesson is now applied**, see [the coverage table](#lessons-applied-hoa-1-to-14).

Everything is plain C++17 with the standard library. There is **no external library to install**, and the code is split into small header modules, so each module can be read and explained on its own. Every function and method has a comment block that says what it does and where it is used, and [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md) lists all of them with their file and line.

---

## Features

### For players
| | |
| :--- | :--- |
| **Accounts** | Create an account and log in. Passwords are stored as a hash, never as plain text. |
| **Store page** | A colored table of all games with price, rating (% recommended) and tags. |
| **Tags, filter and sort** | Browse every tag with its game count, filter by one or more tags, and sort by title, price or rating. |
| **Search** | Search by part of a title or a tag. When nothing matches, the closest title is suggested ("Did you mean ...?"). |
| **Cart and wallet** | Add games to a cart, top up a simulated wallet, and check out (all or nothing). The last cart changes can be **undone**. |
| **Wishlist** | Save games for later and move the whole wishlist to the cart. |
| **Bundle optimizer** | Picks the wishlist games with the best total rating that fit your budget (knapsack). |
| **Reviews** | Recommend or not recommend a game and leave a comment (only owners can review, and posting again updates the review). |
| **Library and downloads** | Every game you own, and an install queue worked by a background thread with a progress bar. |

### Discover
| | |
| :--- | :--- |
| **Featured games** | A carousel of the best rated games. It is a circular list, so `next` after the last game shows the first one. |
| **Browse by genre** | A genre tree (action > shooter, rpg > roguelike, ...) with game counts and the three traversal orders. |
| **Recently viewed and Back / Forward** | The last games you opened, and browser-like back and forward between game pages. |
| **Run all lessons** | A live demo of every lesson, one lesson at a time (`Discover` menu, or start with `--showcase`). |
| **Algorithm lab** | Times the six sorting algorithms, linear against binary search, and plain against memoized Fibonacci. Shows time, comparisons and moves. |

### For developers
| | |
| :--- | :--- |
| **Developer account** | A separate account type, created with a developer access code. |
| **Developer Studio** | Publish a new game (title, description, tags, price) and edit the price, description and tags of **your own** games. |
| **Statistics** | See the owners and the rating of each of your games. |
| **Protected** | Players never see the Studio, and a developer cannot edit games published by someone else. |

### Social
| | |
| :--- | :--- |
| **Friends** | Send, accept, decline and remove friend requests. |
| **Friend network** | Friends at 1, 2 and 3 steps, people you may know (friends of friends), and the shortest chain of friends to another user. |
| **Live chat** | Chat with friends from several terminals at the same time. A background thread shows new messages the moment they arrive. |
| **Unread messages** | Messages that arrive while you are offline are counted at your next login. |

---

## Preview

The store page, filtered by the tag `action` (colors appear in a real terminal):

```
+-----------------------------------------------------------------------------+
|                             G A M E   S T O R E                             |
+-----------------------------------------------------------------------------+
 pat  (id 2, PLAYER)
 Wallet $12.50  |  Cart 0  |  Wishlist 0  |  Unread 0  |  Friend requests 0
-------------------------------------------------------------------------------
 Sort: Newest last (default)   Filter: [action]
+----+-----+--------------------+----------+-----------+----------------------+
| ST |  ID | TITLE              | PRICE    | RATING    | TAGS                 |
+----+-----+--------------------+----------+-----------+----------------------+
|    |   2 | Dungeon Dash       | $9.99    | -         | action, roguelike... |
| O  |   5 | Neon Racer         | $7.50    | 100% (1)  | racing, action, m... |
+----+-----+--------------------+----------+-----------+----------------------+
 ST: O owned, C in cart, W wishlist   Showing 2 of 5 games
```

---

## Quick start

### Requirements
- A C++17 compiler (GCC 9 or newer, Clang, or MinGW-w64 with POSIX threads)
- CMake 3.13 or newer (only if you build with CMake)

### Build and run

**With g++ (Linux, macOS, Windows MinGW):**
```bash
g++ -std=c++17 -pthread main.cpp -o game_store
./game_store
```
On Windows run `game_store.exe`.

**With CMake (CLion or the command line):**
```bash
cmake -S . -B build
cmake --build build
```
The CMake target writes `main` (`main.exe` on Windows) into the project folder, next to the data files.

### Options
| Option | What it does |
| :--- | :--- |
| `--plain` | No colors (for terminals that show strange characters). |
| `--color` | Force the colors, for example when piping into `less -R`. |
| `--showcase` | Runs the demo of **every lesson** (HOA 1 to 14) one after another, then exits. The same demo is in the menu: `Discover` > `Run all lessons`. |
| `--clear` | Clears the screen and the old output between menus even when the console is not a real terminal (for example the CLion Run window, see below). |

Colors are turned off automatically when the output is redirected to a file or when the `NO_COLOR` environment variable is set. In a real terminal the old screen **and the scrollback** are cleared between menus, so only the current content is visible.

**CLion:** the Run window is not a real terminal, so it cannot clear itself. Either add `--clear` to *Run > Edit Configurations > Program arguments*, or turn on *Emulate terminal in output console* in the same dialog.

### First run
The program creates its data files (`steam_*.txt`) in the folder you run it from and adds four sample games. Delete those files to start again from zero.

---

## How to use it

1. **Create an account.** Choose `Create account`, then pick **1) Player** or **2) Developer**. A developer account asks for the access code, which is the constant `DEVELOPER_ACCESS_CODE` in `store.h` (default `dev2026`, change it before you share the program).
2. **Browse.** Use `Sort / filter by tags` to pick a sort order and filter by tags. A game must have **all** of the tags you type, so `action, co-op` narrows the list. `Discover` has the carousel, the genre tree, the history and the Algorithm lab.
3. **Buy.** `Wallet` adds simulated funds (no real payment), `Add a game to the cart` fills the cart, and `Cart / checkout` pays for everything at once (option 4 in the cart undoes the last change).
4. **Install.** `Downloads` queues an owned game. One worker thread installs the games one after another. Installed games are remembered only until you log out.
5. **Review.** After you own a game, use `Post a review / comment`.
6. **Publish (developers).** Open `Developer Studio` to publish or edit games. Give 1 to 8 tags per game, and reuse the existing tags so players can find the game.
7. **Chat.** Add a friend, wait until they accept, then open `Messages`. To try the live chat, start the program in **two terminals in the same folder**, log in as two different users who are friends, and open the chat in both.

---

## Project structure

```
📦 Final-Project---DSA/
├── 🚀 main.cpp          entry point (--plain / --color, screen clearing, starts the store)
│
├── 🏪 Store and screens
│   ├── 🛒 store.h       menus, store page, cart, wishlist, studio, friends, chat screens
│   ├── 🧭 screens.h     Discover, Downloads, Bundle optimizer, Friend network, Algorithm lab
│   ├── 👤 account.h     accounts, roles, wallet, wishlist, owned games
│   ├── 🎮 game.h        games, tags, price, developer, BST by title
│   ├── ⭐ review.h     reviews, comments, recommendation percentage
│   ├── 🔎 browse.h      tag filter, tag index, sorting of the store page
│   ├── 🧺 cart.h        the shopping cart
│   ├── 🤝 friends.h     friend requests and friendships
│   ├── 💬 chat.h        messages and the background listener thread
│   └── 📥 downloads.h   download queue and worker thread
│
├── 🖥️ Terminal and system
│   ├── 🎨 tui.h         colors, banner, aligned tables, menus, progress bar
│   ├── 🔤 textutil.h    UTF-8 aware length and cut
│   └── 🔒 sync.h        file lock and safe file replace for several terminals
│
├── 🧠 Data structures and algorithms
│   ├── 📚 stack.h       ArrayStack and LinkedStack
│   ├── 🚶 queue.h       LinkedQueue and CircularQueue
│   ├── 🔄 ring.h        circular doubly linked list with a cursor
│   ├── ⏪ history.h     Back / Forward, recently viewed, cart undo
│   ├── 🌳 genre.h       genre tree and its traversals
│   ├── 🕸️ graph.h       friend graph, BFS, DFS, suggestions
│   ├── 🧩 dp.h          memoized Fibonacci, knapsack, edit distance
│   ├── 📊 algorithms.h  bubble, selection, insertion, shell, merge, quick sort; linear and binary search
│   └── ⏱️ benchmark.h   timing of sorts and searches with std::chrono
│
├── 🧪 tests/            unit tests of the data structures and algorithms
├── 🛠️ tools/            documentation generator and its cross-check
├── 📖 docs/
│   └── DOCUMENTATION.md every function and method, with file and line
├── ⚙️ CMakeLists.txt    build file
└── 📝 README.md
```

### Data files
All data is kept in text files, one record per line, with `|` between the fields. They are created when the program runs and are ignored by Git (see `.gitignore`). The accounts file contains password **hashes**, so do not commit it.

| File | Fields |
| :--- | :--- |
| `steam_accounts.txt` | `id\|username\|passwordHash\|ownedIds\|walletCents\|wishlistIds\|lastSeenMsg\|role` |
| `steam_games.txt` | `id\|title\|description\|tags\|priceCents\|developerId` |
| `steam_reviews.txt` | `id\|gameId\|userId\|username\|recommended\|comment` |
| `steam_friends.txt` | `fromId\|toId\|accepted` |
| `steam_chat.txt` | `id\|timestamp\|fromId\|fromName\|toId\|text` |

Older files that have fewer fields still load, the missing fields get default values.

---

## Multithreading and several terminals

**Threads inside one program.** There are two worker threads.
- *Chat* (`ChatListener` in `chat.h`): after login it checks the chat file every 300 ms. If the chat with that friend is open it prints the message immediately, otherwise it counts it as unread. The shared unread counters are protected by a `std::mutex`, and printing by a second mutex so lines are not mixed.
- *Downloads* (`DownloadManager` in `downloads.h`): waits on a `std::condition_variable` for a job in the queue, and publishes its progress through `std::atomic` values that the screen reads.

Both threads are stopped and joined at logout and at exit.

**Several programs at once.** Every terminal keeps its own copy of the data, so a plain "save everything" would overwrite the changes of the others. Before any change, the program (`Transaction` in `store.h`):

1. takes a file lock (`FileLock` in `sync.h`),
2. reads all the files again,
3. applies the change,
4. writes each file through a temporary file and renames it, then releases the lock.

---

## Lessons applied (HOA 1 to 14)

| HOA | Lesson | Status | Where it is used |
| :---: | :--- | :---: | :--- |
| 1 | Functions | ✅ Applied | The whole program is built from small functions, with references, default parameters and templates. |
| 2.1 | Arrays and pointers | ✅ Applied | Dynamic arrays with `new[]` / `delete[]` in `ArrayStack` and `CircularQueue`, array based sorting and searching in `algorithms.h`, pointers and cleanup for every structure. |
| 3 | Linked lists | ✅ Applied | Singly linked lists (games, accounts, reviews, cart, wishlist, friends), `LinkedStack`, `LinkedQueue`, and the **circular doubly linked** `RingList` of the Featured carousel. |
| 4 | Stacks | ✅ Applied | `ArrayStack` and `LinkedStack`: Back / Forward, cart undo, iterative depth first search. |
| 5 | Queues | ✅ Applied | `LinkedQueue` (download queue, BFS, level order) and `CircularQueue` (recently viewed). |
| 6 | Searching | ✅ Applied | Linear search, binary search on a sorted array, `partitionPoint` (lower / upper bound), BST search by title and username. |
| 7, 8 | Sorting | ✅ Applied | Bubble, selection, insertion, shell, merge and quick sort, all as templates, with the store page sorted by insertion sort. |
| 9 | Trees | ✅ Applied | BST insert / search / free, a **general tree** of genres, preorder, postorder and level order traversals. |
| 10 | Graphs | ✅ Applied | Friend network as adjacency list and adjacency matrix, BFS (distances, shortest chain), DFS (friend group), friend suggestions. |
| 11 | Binary search timing | ✅ Applied | Algorithm lab: comparisons and time of linear against binary search. |
| 12 | Dynamic programming | ✅ Applied | Memoized Fibonacci, knapsack bundle optimizer, edit distance for "Did you mean?". |
| 13 | Multithreading | ✅ Applied | Chat listener thread, download worker thread, mutexes, condition variable, atomics, join at logout. |
| 14 | Sort and search benchmarks | ✅ Applied | Algorithm lab: all six sorts on random, sorted, reversed and few-unique data, with time, comparisons and moves (`std::chrono`). |

---

## Documentation

**[docs/DOCUMENTATION.md](docs/DOCUMENTATION.md)** has:
- a master index of all functions and methods (name, kind, file and line),
- a table of every file with the lessons it shows,
- the include diagram and a menu map (what calls what),
- a reference entry for every function: signature, what it does, what it returns, where it is used, and what it calls.

The same information is in the source. Every file starts with a **FUNCTION INDEX** (name, line, kind), and every function has a block above it:

```
//--- BrowseHistory::visit ---------------------------------------------------
// What    : A page was opened by the user: the current page goes on the back stack ...
// Used by : doViewGame (store.h)
//----------------------------------------------------------------------------
```

`What` and `Returns` are written by hand. `Used by` and the indexes are written by the tool, so they do not go out of date:

```bash
python3 tools/docgen.py --update-source   # rewrite the blocks and indexes, and docs/DOCUMENTATION.md
python3 tools/docgen.py --check           # change nothing, fail when something is out of date or has no description
```

The tool needs only Python 3. If you also run `pip install libclang`, the "Used by" of class methods is exact (about half a minute); without it methods are matched by name. `python3 tools/crosscheck_clang.py` compares the tool's function list and call graph with libclang. Only comment lines are ever changed, never the code.

**Adding a function:** write it, run `--update-source`, then replace the `TODO describe what it does` line in its new block with a sentence, and run `--update-source` again.

---

## Testing

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
cd build && ctest
```

| Test | Checks |
| :--- | :--- |
| `test_ds` | All six sorts against `std::stable_sort` (empty, one item, duplicates, sorted, reversed, stability), binary search, lower / upper bound, both stacks, both queues, the ring list (copy, wrap, remove), UTF-8 helpers. |
| `test_graph_dp` | Friend graph (BFS distances, shortest chain, DFS group, suggestions), Fibonacci, knapsack, edit distance. |
| `test_tree_hist` | Genre tree traversals and totals, Back / Forward, recently viewed, cart undo. |
| `test_dl` | Download queue order, duplicate rejection, stopping the worker in the middle of a download. |
| `test_bench` | Every benchmark result is sorted, early exit of bubble and insertion sort, quadratic sorts skipped for large inputs, binary search count. |

The program itself was also run with the address, undefined-behavior and thread sanitizers, against data files from the previous version, and from six terminals at once (no lost accounts, exact wallet total).

---

## Known limitations

- The chat works between terminals on **one computer** that share a folder. It is not a network chat.
- The wallet is simulated, and there are no refunds.
- Passwords use a simple hash (djb2), which is fine for coursework but is **not** real security. The developer access code is also stored in the source code.
- The cart, the install list, the history and the undo list live only in memory and are not saved between logins.
- Downloads are a simulation: nothing is copied, only the progress is counted.
- Table columns count characters correctly for accents, but emoji and East Asian double-width characters are still drawn wider than one column by most terminals.
- Tested with GCC on Linux (including address, undefined-behavior and thread sanitizers). Windows builds with MinGW-w64 are expected to work but were not tested.

---

## Authors

**CPE 010 - Data Structures and Algorithms, Final Project**, Technological Institute of the Philippines

- Mendoza, Roy Gabriel
- Pangilinan, Bienvenido B.
- Pasion, Al Twain D.
- Reyes, Eli Justin L.
- Torres, Carl Andrei D.


<!-- add the other group members here -->
