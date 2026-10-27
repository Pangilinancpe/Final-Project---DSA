<div align="center">
<img width="360" alt="Game Store logo" src="https://github.com/user-attachments/assets/2789f12e-50c7-443f-8c3a-4bbfd37bd2aa" />

  # Game Store
 
**A Steam-style game platform for the terminal, written in C++17**
 
Accounts &bull; Developer Studio &bull; Tags and sorting &bull; Cart and wallet &bull; Friends and live chat
 
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
  <a href="#roadmap">Roadmap</a>
</p>
</div>
 
## About
 
Game Store is the **final project of CPE 010 - Data Structures and Algorithms** (Technological Institute of the Philippines). It is a mock-up of a game storefront like Steam that runs in the console. The goal of the project is to apply the lessons of **Hands-On Activities 1 to 14** inside one real program instead of separate exercises.
 
Everything is plain C++17 with the standard library. There is **no external library to install**, and the code is split into small header modules, so each module can be read and explained on its own.
 
---
 
## Features
 
### For players
| | |
| :--- | :--- |
| **Accounts** | Create an account and log in. Passwords are stored as a hash, never as plain text. |
| **Store page** | A colored table of all games with price, rating (% recommended) and tags. |
| **Tags, filter and sort** | Browse every tag with its game count, filter by one or more tags, and sort by title, price or rating. |
| **Search** | Search by part of a title or a tag. |
| **Cart and wallet** | Add games to a cart, top up a simulated wallet, and check out. A checkout is all or nothing. |
| **Wishlist** | Save games for later and move the whole wishlist to the cart. |
| **Reviews** | Recommend or not recommend a game and leave a comment (only owners can review, and posting again updates the review). |
| **Library** | Every game you own. |
 
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
- CMake 3.10 or newer (only if you build with CMake)
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
./build/game_store
```
 
### Options
| Option | What it does |
| :--- | :--- |
| `--plain` | No colors (for terminals that show strange characters). |
| `--color` | Force the colors, for example when piping into `less -R`. |
 
Colors are turned off automatically when the output is redirected to a file or when the `NO_COLOR` environment variable is set.
 
### First run
The program creates its data files (`steam_*.txt`) in the folder you run it from and adds four sample games. Delete those files to start again from zero.
 
---
 
## How to use it
 
1. **Create an account.** Choose `Create account`, then pick **1) Player** or **2) Developer**. A developer account asks for the access code, which is the constant `DEVELOPER_ACCESS_CODE` in `store.h` (default `dev2026`, change it before you share the program).
2. **Browse.** Use `Sort / filter by tags` to pick a sort order and filter by tags. A game must have **all** of the tags you type, so `action, co-op` narrows the list.
3. **Buy.** `Wallet` adds simulated funds (no real payment), `Add a game to the cart` fills the cart, and `Cart / checkout` pays for everything at once.
4. **Review.** After you own a game, use `Post a review / comment`.
5. **Publish (developers).** Open `Developer Studio` to publish or edit games. Give 1 to 8 tags per game, and reuse the existing tags so players can find the game.
6. **Chat.** Add a friend, wait until they accept, then open `Messages`. To try the live chat, start the program in **two terminals in the same folder**, log in as two different users who are friends, and open the chat in both.
---
 
## Project structure
 
```
Final-Project---DSA/
|-- main.cpp        entry point (reads --plain / --color, starts the store)
|-- store.h         the store: menus, store page, cart, wishlist, studio, friends, chat screens
|-- account.h       accounts, roles (player / developer), wallet, wishlist, owned games
|-- game.h          games, tags, price, developer, BST by title
|-- review.h        reviews, comments and the recommendation percentage
|-- browse.h        tag filter, tag index and sorting (insertion sort)
|-- cart.h          the shopping cart
|-- friends.h       friend requests and friendships
|-- chat.h          messages and the background listener thread
|-- sync.h          file lock and safe file replace for several terminals
|-- tui.h           colors, banner, aligned tables and menus
|-- CMakeLists.txt  build file
`-- README.md
```
 
| Module | Data structure / technique | Used for |
| :--- | :--- | :--- |
| `game.h` | Singly linked list + **BST** keyed by title | Catalog and fast title lookup |
| `account.h` | Singly linked list + **BST** keyed by username, linked list of owned ids | Login, duplicate check, library |
| `review.h` | Singly linked list | Reviews and rating statistics |
| `cart.h` | Singly linked list | Shopping cart |
| `friends.h` | Singly linked list of links between users | Friend requests and friendships |
| `browse.h` | **Insertion sort** (stable) over a list of pointers, `std::map` for the tag index | Sorting and tag filtering |
| `chat.h` | `std::thread`, `std::mutex`, `std::atomic` | Live chat |
| `sync.h` | RAII lock object | Safe use from several terminals |
 
### Data files
All data is kept in text files, one record per line, with `|` between the fields. They are created when the program runs and are ignored by Git (see `.gitignore`).
 
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
 
**Threads inside one program.** After login, a worker thread (`ChatListener` in `chat.h`) checks the chat file every 300 ms. If the chat with that friend is open it prints the message immediately, otherwise it counts it as unread and prints a notice. The thread never touches the accounts, games or reviews. The only data shared with the main thread (the unread counters) is protected by a `std::mutex`, and printing is protected by a second mutex so lines are not mixed. The thread is stopped and joined at logout and at exit.
 
**Several programs at once.** Every terminal keeps its own copy of the data, so a plain "save everything" would overwrite the changes of the others. Before any change, the program (`Transaction` in `store.h`):
 
1. takes a file lock (`FileLock` in `sync.h`),
2. reads all the files again,
3. applies the change,
4. writes each file through a temporary file and renames it, then releases the lock.
---
 
## Lessons applied (HOA 1 to 14)
 
The project is meant to use every lesson of the course. Status of each one:
 
| HOA | Lesson | Status | Where it is used |
| :---: | :--- | :---: | :--- |
| 1 | Functions | ✅ Applied | The whole program is built from small functions, with references and default parameters. |
| 2.1 | Arrays and pointers | 🟡 Partial | Pointers with `new` / `delete` and a cleanup function for every structure. Arrays are only used for the table widths. |
| 3 | Linked lists | 🟡 Partial | Singly linked lists for games, accounts, reviews, cart, wishlist, friends. Doubly and circular lists are still to come. |
| 4 | Stacks | ⬜ Planned | |
| 5 | Queues | ⬜ Planned | |
| 6 | Searching | 🟡 Partial | Linear search by id and BST search by title and username. Binary search on a sorted array is still to come. |
| 7, 8 | Sorting | 🟡 Partial | Insertion sort for the store page. Bubble, selection, shell, merge and quick sort are still to come. |
| 9 | Trees | 🟡 Partial | BST insert, search and free. Traversals and a general tree are still to come. |
| 10 | Graphs | ⬜ Planned | |
| 11 | Binary search timing | ⬜ Planned | |
| 12 | Dynamic programming | ⬜ Planned | |
| 13 | Multithreading | ✅ Applied | Chat listener thread, mutexes, atomics, join at logout. |
| 14 | Sort and search benchmarks | ⬜ Planned | |
 
---
 
## Roadmap
 
| Step | Feature | HOA |
| :---: | :--- | :---: |
| 1 | **Benchmark screen:** generate a catalog of thousands of games and time every sorting algorithm and linear against binary search with `std::chrono`. | 6, 7, 8, 11, 14 |
| 2 | **Back button and undo** with stacks, a **download manager** with a job queue worked by threads, and a **recently viewed** list with a circular queue. | 3, 4, 5, 13 |
| 3 | **Friend suggestions** and groups: friends as an adjacency list, with BFS and DFS. | 10 |
| 4 | **Genre tree:** genre, sub-genre, games, with pre-order, in-order and post-order printing. | 9 |
| 5 | **Bundle optimizer** (best value for the wallet, knapsack with memoization) and **"Did you mean?"** search (edit distance). | 12 |
 
---
 
## Known limitations
 
- The chat works between terminals on **one computer** that share a folder. It is not a network chat.
- The wallet is simulated, and there are no refunds.
- Passwords use a simple hash (djb2), which is fine for coursework but is **not** real security. The developer access code is also stored in the source code.
- The cart is not saved between logins.
- Titles and tags with non-English characters can misalign the table (
thread sanitizers). Windows builds with MinGW-w64 are expected to work but were not tested.
---
 
## Authors
 
**CPE 010 - Data Structures and Algorithms, Final Project**, Technological Institute of the Philippines
 
- Mendoza, Roy Gabriel
- Pangilinan, Bienvenido B.
- Pasion, Al Twain D.
- Reyes, Eli Justin L.
- Torres, Carl Andrei D.
<!-- add the other group members here -->
 
