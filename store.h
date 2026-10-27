#ifndef STEAM_STORE_H
#define STEAM_STORE_H

#include "account.h"
#include "game.h"
#include "review.h"
#include "friends.h"
#include "chat.h"
#include "cart.h"
#include "sync.h"

//==========================================
// STORE MODULE (main page)
// Ties all modules together: login, the store page with prices and ratings,
// cart + checkout with a wallet, wishlist, reviews, publishing games,
// friends and live chat.
//
// SEVERAL TERMINALS: every terminal keeps its own copy of the data in memory,
// so before a change is made the program
//   1. takes the shared lock       (FileLock "steam_store")
//   2. re-reads all the files      (so it sees what other terminals changed)
//   3. applies the change
//   4. writes the files and releases the lock
// That is what the Transaction class below does.
//==========================================

const std::string ACCOUNTS_FILE = "steam_accounts.txt";
const std::string GAMES_FILE    = "steam_games.txt";
const std::string REVIEWS_FILE  = "steam_reviews.txt";
const std::string FRIENDS_FILE  = "steam_friends.txt";
const std::string CHAT_FILE     = "steam_chat.txt";

// true  = only owners of a game may review it (like the real Steam)
// false = any logged in user may comment on any game
const bool REQUIRE_OWNERSHIP_TO_REVIEW = true;

// limits for the simulated wallet top-up (in cents)
const int MIN_TOPUP_CENTS = 100;       // $1.00
const int MAX_TOPUP_CENTS = 50000;     // $500.00

struct Store {
    GameCatalog games;
    AccountRegistry accounts;
    ReviewList reviews;
    FriendList friends;
    Cart cart;                          // only lives during the login session
    ChatListener chat;                  // owns the background thread
    AccountNode* current = nullptr;     // the logged in user, nullptr = guest
};

//==========================
// input helpers (getline based, so spaces are allowed and bad input can not loop forever)
//==========================
bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt << std::flush;
    if (!std::getline(std::cin, out)) return false;      // end of input
    return true;
}

// returns -1 for anything that is not a whole number, -2 when the input ended
int readInt(const std::string& prompt) {
    std::string s;
    if (!readLine(prompt, s)) return -2;
    s = gameTrim(s);
    if (s.empty()) return -1;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
    }
    if (s.size() > 9) return -1;
    return std::atoi(s.c_str());
}

// "12", "12.5", "$12.50" -> cents. returns -1 when it is not a valid amount
int parseMoney(std::string s) {
    s = gameTrim(s);
    if (!s.empty() && s[0] == '$') s = s.substr(1);
    if (s.empty()) return -1;

    size_t dot = s.find('.');
    std::string whole = (dot == std::string::npos) ? s : s.substr(0, dot);
    std::string frac = (dot == std::string::npos) ? "" : s.substr(dot + 1);
    if (whole.empty() && frac.empty()) return -1;
    if (whole.size() > 6 || frac.size() > 2) return -1;

    for (size_t i = 0; i < whole.size(); i++) if (whole[i] < '0' || whole[i] > '9') return -1;
    for (size_t i = 0; i < frac.size(); i++) if (frac[i] < '0' || frac[i] > '9') return -1;

    int cents = std::atoi(whole.c_str()) * 100;
    if (frac.size() == 1) frac += "0";
    if (!frac.empty()) cents += std::atoi(frac.c_str());
    return cents;
}

//==========================
// reading / writing the shared files
//==========================
void freeStoreData(Store& st) {
    freeCatalog(st.games);
    freeAccounts(st.accounts);
    freeReviews(st.reviews);
    freeFriends(st.friends);
    st.current = nullptr;
}

// writes one file through "<file>.tmp" so other terminals never read a half written file
template <typename T, typename Saver>
bool saveAtomic(const T& data, const std::string& file, Saver saver) {
    std::string tmp = file + ".tmp";
    if (!saver(data, tmp)) return false;
    return replaceFile(tmp, file);
}

bool saveStore(const Store& st) {
    bool a = saveAtomic(st.accounts, ACCOUNTS_FILE, saveAccounts);
    bool g = saveAtomic(st.games, GAMES_FILE, saveGames);
    bool r = saveAtomic(st.reviews, REVIEWS_FILE, saveReviews);
    bool f = saveAtomic(st.friends, FRIENDS_FILE, saveFriends);
    bool ok = a && g && r && f;
    if (!ok) std::cout << "  (warning: could not write the data files)\n";
    return ok;
}

// throws away the copy in memory and reads the files again.
// the same user stays logged in (the account is looked up again by its id,
// because every pointer into the old data is gone after this call)
void reloadStore(Store& st) {
    int userId = (st.current != nullptr) ? st.current->userId : 0;

    freeStoreData(st);
    loadAccounts(st.accounts, ACCOUNTS_FILE);
    loadGames(st.games, GAMES_FILE);
    loadReviews(st.reviews, REVIEWS_FILE);
    loadFriends(st.friends, FRIENDS_FILE);

    if (userId != 0) st.current = findAccountById(st.accounts, userId);
}

//==========================
// Transaction: lock + reload at the start, save with commit()
// If commit() is never called (the action was cancelled) nothing is written.
// IMPORTANT: every GameNode* / AccountNode* taken BEFORE the transaction is invalid
// inside it, so only ids are carried over and the nodes are looked up again.
//==========================
class Transaction {
private:
    Store& st;
    FileLock lock;
    bool ready;

public:
    explicit Transaction(Store& store) : st(store), lock("steam_store"), ready(lock.locked()) {
        if (ready) reloadStore(st);
        else std::cout << "  The store is busy right now, please try again.\n";
    }

    bool ok() const { return ready; }

    bool commit() { return ready && saveStore(st); }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
};

//==========================
// first run: a few sample games so the store is not empty
//==========================
void seedGames(Store& st) {
    createGame(st.games, "Space Miner", "Dig asteroids, sell ore and upgrade your ship.", "simulation, space, singleplayer", 1499);
    createGame(st.games, "Dungeon Dash", "Race through procedurally generated dungeons.", "action, roguelike", 999);
    createGame(st.games, "Farm Valley", "Grow crops, raise animals and befriend the village.", "farming, relaxing, co-op", 1999);
    createGame(st.games, "Block Builders", "Build anything with friends in an open sandbox.", "sandbox, multiplayer, creative", 0);
}

void initStore(Store& st) {
    reloadStore(st);
    if (st.games.count == 0) {
        Transaction tx(st);                  // reloads again, in case another terminal seeded first
        if (tx.ok() && st.games.count == 0) {
            seedGames(st);
            tx.commit();
        }
    }
}

//==========================
// store page: every game, with price and rating line
//==========================
void printGameLine(const Store& st, const GameNode* g) {
    int total, rec;
    reviewStats(st.reviews, g->gameId, total, rec);

    std::cout << "  [" << g->gameId << "] " << g->title << "  -  " << priceText(g->priceCents);
    if (total == 0) std::cout << "  -  no reviews yet";
    else std::cout << "  -  " << recommendPercent(st.reviews, g->gameId) << "% recommended (" << total << " review" << (total == 1 ? "" : "s") << ")";
    if (st.current != nullptr) {
        if (ownsGame(st.current, g->gameId)) std::cout << "  [OWNED]";
        else if (inCart(st.cart, g->gameId)) std::cout << "  [IN CART]";
        else if (onWishlist(st.current, g->gameId)) std::cout << "  [WISHLISTED]";
    }
    std::cout << "\n      Tags: " << (g->tags.empty() ? "(none)" : g->tags) << "\n";
}

void showStorePage(const Store& st) {
    std::cout << "\n================ STORE ================\n";
    if (st.current != nullptr) {
        std::cout << "Logged in as: " << st.current->username << " (id " << st.current->userId << ")\n"
                  << "Wallet: " << moneyText(st.current->walletCents)
                  << "  |  Cart: " << st.cart.count
                  << "  |  Wishlist: " << st.current->wishCount
                  << "  |  Unread messages: " << st.chat.totalUnread()
                  << "  |  Friend requests: " << countIncoming(st.friends, st.current->userId) << "\n";
    } else {
        std::cout << "Browsing as guest (log in to buy and review)\n";
    }
    std::cout << "---------------------------------------\n";
    if (st.games.count == 0) std::cout << "  (no games yet)\n";
    for (GameNode* g = st.games.head; g != nullptr; g = g->next) printGameLine(st, g);
    std::cout << "=======================================\n";
}

//==========================
// one game page: details + all reviews
//==========================
void showGamePage(const Store& st, const GameNode* g) {
    std::cout << "\n---------- GAME PAGE ----------\n";
    displayGameDetails(g);
    int total, rec;
    reviewStats(st.reviews, g->gameId, total, rec);
    if (total > 0) std::cout << "      Rating: " << recommendPercent(st.reviews, g->gameId) << "% recommended (" << rec << " of " << total << ")\n";
    std::cout << "  Reviews:\n";
    displayReviewsForGame(st.reviews, g->gameId);
    std::cout << "-------------------------------\n";
}

//==========================
// pick a game by id or title; nullptr if not found
//==========================
GameNode* askForGame(const Store& st) {
    std::string in;
    if (!readLine("Game id or title: ", in)) return nullptr;
    in = gameTrim(in);

    bool allDigits = !in.empty();
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i] < '0' || in[i] > '9') allDigits = false;
    }
    GameNode* g = nullptr;
    if (allDigits && in.size() <= 9) g = findGameById(st.games, std::atoi(in.c_str()));
    if (g == nullptr) g = findGameByTitle(st.games, in);
    if (g == nullptr) std::cout << "  Game not found.\n";
    return g;
}

//==========================
// account actions
//==========================
void doRegister(Store& st) {
    std::string user, pass, confirm;
    std::cout << "\n-- Create account --\n(username: 3-20 letters, digits or _ ; password: at least 4 characters)\n";
    if (!readLine("Username: ", user)) return;
    if (!readLine("Password: ", pass)) return;
    if (!readLine("Confirm password: ", confirm)) return;

    if (pass != confirm) { std::cout << "  Passwords do not match.\n"; return; }
    if (!validUsername(user)) { std::cout << "  Invalid username.\n"; return; }
    if (pass.size() < 4) { std::cout << "  Password is too short.\n"; return; }

    Transaction tx(st);
    if (!tx.ok()) return;
    AccountNode* acc = createAccount(st.accounts, user, pass);
    if (acc == nullptr) { std::cout << "  That username is already taken.\n"; return; }

    int newId = acc->userId;
    if (tx.commit()) std::cout << "  Account created! Your user id is " << newId << ". You can log in now.\n";
}

void doLogin(Store& st) {
    std::string user, pass;
    std::cout << "\n-- Log in --\n";
    if (!readLine("Username: ", user)) return;
    if (!readLine("Password: ", pass)) return;

    AccountNode* acc = loginAccount(st.accounts, user, pass);
    if (acc == nullptr) { std::cout << "  Wrong username or password.\n"; return; }
    st.current = acc;
    clearCart(st.cart);
    st.chat.start(acc->userId, CHAT_FILE, acc->lastSeenMsg);     // starts the background thread
    std::cout << "  Welcome back, " << acc->username << "!\n";
    int unread = st.chat.totalUnread();
    if (unread > 0) std::cout << "  You have " << unread << " unread message" << (unread == 1 ? "" : "s") << ".\n";
}

void doLogout(Store& st) {
    st.chat.stop();                      // waits for the background thread to finish
    clearCart(st.cart);
    st.current = nullptr;
    std::cout << "  Logged out.\n";
}

//==========================
// browsing
//==========================
void doViewGame(Store& st) {
    GameNode* g = askForGame(st);
    if (g != nullptr) showGamePage(st, g);
}

void doSearch(const Store& st) {
    std::string key;
    if (!readLine("Search (title or tag): ", key)) return;
    key = gameTrim(key);
    if (key.empty()) { std::cout << "  Nothing to search for.\n"; return; }

    int found = 0;
    for (GameNode* g = st.games.head; g != nullptr; g = g->next) {
        if (gameMatches(g, key)) { printGameLine(st, g); found++; }
    }
    if (found == 0) std::cout << "  No games match \"" << key << "\".\n";
}

void doLibrary(const Store& st) {
    std::cout << "\n-- " << st.current->username << "'s library (" << st.current->ownedCount << " game"
              << (st.current->ownedCount == 1 ? "" : "s") << ") --\n";
    if (st.current->ownedCount == 0) { std::cout << "  (empty, buy a game from the store)\n"; return; }
    for (OwnedNode* o = st.current->owned; o != nullptr; o = o->next) {
        GameNode* g = findGameById(st.games, o->gameId);
        if (g != nullptr) std::cout << "  [" << g->gameId << "] " << g->title << "\n";
    }
}

//==========================
// wallet
//==========================
void doWallet(Store& st) {
    std::cout << "\n-- Wallet --\nBalance: " << moneyText(st.current->walletCents)
              << "\n(adding funds is simulated, no real payment is made)\n";
    std::string in;
    if (!readLine("Amount to add (e.g. 10 or 9.99, blank to cancel): ", in)) return;
    if (gameTrim(in).empty()) return;

    int cents = parseMoney(in);
    if (cents < MIN_TOPUP_CENTS || cents > MAX_TOPUP_CENTS) {
        std::cout << "  Enter an amount from " << moneyText(MIN_TOPUP_CENTS) << " to " << moneyText(MAX_TOPUP_CENTS) << ".\n";
        return;
    }

    Transaction tx(st);
    if (!tx.ok()) return;
    if (!addFunds(st.current, cents)) { std::cout << "  The wallet can not hold more than " << moneyText(MAX_WALLET_CENTS) << ".\n"; return; }
    if (tx.commit()) std::cout << "  Added " << moneyText(cents) << ". New balance: " << moneyText(st.current->walletCents) << "\n";
}

//==========================
// cart + checkout
//==========================
void doAddToCart(Store& st) {
    GameNode* g = askForGame(st);
    if (g == nullptr) return;
    if (ownsGame(st.current, g->gameId)) { std::cout << "  You already own " << g->title << ".\n"; return; }
    if (!addToCart(st.cart, g->gameId)) { std::cout << "  " << g->title << " is already in your cart.\n"; return; }
    std::cout << "  " << g->title << " (" << priceText(g->priceCents) << ") added to your cart.\n";
}

void showCart(const Store& st, int& total) {
    total = 0;
    std::cout << "\n-- Cart (" << st.cart.count << " item" << (st.cart.count == 1 ? "" : "s") << ") --\n";
    if (st.cart.count == 0) { std::cout << "  (empty)\n"; return; }
    for (CartNode* c = st.cart.head; c != nullptr; c = c->next) {
        GameNode* g = findGameById(st.games, c->gameId);
        if (g == nullptr) continue;
        std::cout << "  [" << g->gameId << "] " << g->title << "  -  " << priceText(g->priceCents) << "\n";
        total += g->priceCents;
    }
    std::cout << "  Total: " << moneyText(total) << "     Wallet: " << moneyText(st.current->walletCents) << "\n";
}

// all or nothing: either every game is bought, or nothing changes
void doCheckout(Store& st) {
    Transaction tx(st);
    if (!tx.ok()) return;

    int total = 0;
    int toBuy = 0;
    for (CartNode* c = st.cart.head; c != nullptr; c = c->next) {
        GameNode* g = findGameById(st.games, c->gameId);
        if (g == nullptr || ownsGame(st.current, c->gameId)) continue;   // got it from another terminal meanwhile
        total += g->priceCents;
        toBuy++;
    }
    if (toBuy == 0) { std::cout << "  Nothing to buy.\n"; clearCart(st.cart); return; }

    if (st.current->walletCents < total) {
        std::cout << "  Not enough funds: total " << moneyText(total) << ", wallet " << moneyText(st.current->walletCents)
                  << ". Add funds in the Wallet menu.\n";
        return;
    }

    spendFunds(st.current, total);
    std::cout << "\n-- Receipt --\n";
    for (CartNode* c = st.cart.head; c != nullptr; c = c->next) {
        GameNode* g = findGameById(st.games, c->gameId);
        if (g == nullptr || ownsGame(st.current, c->gameId)) continue;
        addOwnedGame(st.current, g->gameId);
        removeWish(st.current, g->gameId);                 // no longer wished for
        std::cout << "  " << g->title << "  " << priceText(g->priceCents) << "\n";
    }
    std::cout << "  Paid: " << moneyText(total) << "   Balance left: " << moneyText(st.current->walletCents) << "\n";

    if (tx.commit()) clearCart(st.cart);
}

void doCart(Store& st) {
    while (true) {
        int total = 0;
        showCart(st, total);
        std::cout << "1. Checkout\n2. Remove an item\n3. Empty the cart\n0. Back\n";
        int c = readInt("Choice: ");
        if (c == -2 || c == 0) return;
        if (c == 1) { doCheckout(st); return; }
        if (c == 2) {
            GameNode* g = askForGame(st);
            if (g != nullptr) std::cout << (removeFromCart(st.cart, g->gameId) ? "  Removed.\n" : "  That game is not in your cart.\n");
        } else if (c == 3) {
            clearCart(st.cart);
            std::cout << "  Cart emptied.\n";
        } else {
            std::cout << "  Invalid choice.\n";
        }
    }
}

//==========================
// wishlist
//==========================
void showWishlist(const Store& st) {
    std::cout << "\n-- Wishlist (" << st.current->wishCount << ") --\n";
    if (st.current->wishCount == 0) { std::cout << "  (empty)\n"; return; }
    for (WishNode* w = st.current->wishlist; w != nullptr; w = w->next) {
        GameNode* g = findGameById(st.games, w->gameId);
        if (g != nullptr) std::cout << "  [" << g->gameId << "] " << g->title << "  -  " << priceText(g->priceCents) << "\n";
    }
}

void doWishlist(Store& st) {
    while (true) {
        showWishlist(st);
        std::cout << "1. Add a game\n2. Remove a game\n3. Move everything to the cart\n0. Back\n";
        int c = readInt("Choice: ");
        if (c == -2 || c == 0) return;

        if (c == 1 || c == 2) {
            GameNode* g = askForGame(st);
            if (g == nullptr) continue;
            int gid = g->gameId;
            std::string title = g->title;
            if (c == 1 && ownsGame(st.current, gid)) { std::cout << "  You already own " << title << ".\n"; continue; }

            Transaction tx(st);
            if (!tx.ok()) continue;
            bool changed = (c == 1) ? addWish(st.current, gid) : removeWish(st.current, gid);
            if (!changed) { std::cout << (c == 1 ? "  Already on your wishlist.\n" : "  That game is not on your wishlist.\n"); continue; }
            if (tx.commit()) std::cout << (c == 1 ? "  Added " : "  Removed ") << title << (c == 1 ? " to" : " from") << " your wishlist.\n";
        } else if (c == 3) {
            int moved = 0;
            for (WishNode* w = st.current->wishlist; w != nullptr; w = w->next) {
                if (!ownsGame(st.current, w->gameId) && addToCart(st.cart, w->gameId)) moved++;
            }
            std::cout << "  " << moved << " game" << (moved == 1 ? "" : "s") << " added to the cart.\n";
        } else {
            std::cout << "  Invalid choice.\n";
        }
    }
}

//==========================
// reviews and publishing
//==========================
void doReview(Store& st) {
    GameNode* g = askForGame(st);
    if (g == nullptr) return;
    int gid = g->gameId;
    std::string title = g->title;

    if (REQUIRE_OWNERSHIP_TO_REVIEW && !ownsGame(st.current, gid)) {
        std::cout << "  You need to own " << title << " before reviewing it.\n";
        return;
    }
    if (findReview(st.reviews, gid, st.current->userId) != nullptr)
        std::cout << "  (you already reviewed this game, posting again will update your review)\n";

    std::string rec, comment;
    if (!readLine("Recommend " + title + "? (y/n): ", rec)) return;
    rec = gameToLower(gameTrim(rec));
    if (rec != "y" && rec != "n") { std::cout << "  Please answer y or n.\n"; return; }
    if (!readLine("Comment: ", comment)) return;

    Transaction tx(st);
    if (!tx.ok()) return;
    int result = postReview(st.reviews, gid, st.current->userId, st.current->username, rec == "y", comment);
    if (result == 0) { std::cout << "  The comment can not be empty.\n"; return; }
    if (tx.commit()) std::cout << (result == 1 ? "  Review posted!\n" : "  Review updated!\n");
}

void doPublish(Store& st) {
    std::string title, desc, tags, priceIn;
    std::cout << "\n-- Publish a new game --\n";
    if (!readLine("Title: ", title)) return;
    if (!readLine("Description: ", desc)) return;
    if (!readLine("Tags (comma separated): ", tags)) return;
    if (!readLine("Price (e.g. 9.99, or 0 for free): ", priceIn)) return;

    int price = parseMoney(priceIn);
    if (price < 0) { std::cout << "  Invalid price.\n"; return; }

    Transaction tx(st);
    if (!tx.ok()) return;
    GameNode* g = createGame(st.games, title, desc, tags, price);
    if (g == nullptr) { std::cout << "  Title is empty or a game with that title already exists.\n"; return; }

    addOwnedGame(st.current, g->gameId);     // the publisher gets a copy
    int newId = g->gameId;
    std::string newTitle = g->title;
    if (tx.commit()) std::cout << "  Published! \"" << newTitle << "\" has game id " << newId << " and costs " << priceText(price) << ".\n";
}

//==========================
// friends
//==========================
// the other user of a friendship record
int otherUser(const FriendNode* link, int me) {
    return link->fromId == me ? link->toId : link->fromId;
}

std::string nameOf(const Store& st, int userId) {
    AccountNode* acc = findAccountById(st.accounts, userId);
    return acc == nullptr ? "(unknown)" : acc->username;
}

void showFriends(const Store& st) {
    int me = st.current->userId;
    int friendsCount = 0;
    std::cout << "\n-- Friends --\n";
    for (FriendNode* f = st.friends.head; f != nullptr; f = f->next) {
        if (!f->accepted || (f->fromId != me && f->toId != me)) continue;
        int other = otherUser(f, me);
        int unread = st.chat.unreadFrom(other);
        std::cout << "  " << nameOf(st, other);
        if (unread > 0) std::cout << "  (" << unread << " unread)";
        std::cout << "\n";
        friendsCount++;
    }
    if (friendsCount == 0) std::cout << "  (no friends yet)\n";

    int pendingShown = 0;
    for (FriendNode* f = st.friends.head; f != nullptr; f = f->next) {
        if (f->accepted) continue;
        if (f->toId == me) {
            if (pendingShown == 0) std::cout << "Requests waiting for you:\n";
            std::cout << "  " << nameOf(st, f->fromId) << "\n";
            pendingShown++;
        }
    }
    for (FriendNode* f = st.friends.head; f != nullptr; f = f->next) {
        if (!f->accepted && f->fromId == me) std::cout << "Request sent to: " << nameOf(st, f->toId) << " (waiting)\n";
    }
}

void doFriends(Store& st) {
    while (true) {
        reloadStore(st);
        showFriends(st);
        std::cout << "1. Send a friend request\n2. Accept a request\n3. Decline a request / remove a friend\n0. Back\n";
        int c = readInt("Choice: ");
        if (c == -2 || c == 0) return;
        if (c < 1 || c > 3) { std::cout << "  Invalid choice.\n"; continue; }

        std::string name;
        if (!readLine("Username: ", name)) return;
        name = gameTrim(name);

        Transaction tx(st);
        if (!tx.ok()) continue;
        AccountNode* other = findAccountByName(st.accounts, name);
        if (other == nullptr) { std::cout << "  No user called \"" << name << "\".\n"; continue; }
        int me = st.current->userId;
        int otherId = other->userId;
        std::string otherName = other->username;
        bool changed = false;

        if (c == 1) {
            int r = sendFriendRequest(st.friends, me, otherId);
            if (r == FR_SELF) std::cout << "  You can not add yourself.\n";
            else if (r == FR_ALREADY) std::cout << "  You are already friends with " << otherName << ".\n";
            else if (r == FR_PENDING) std::cout << "  You already sent a request to " << otherName << ".\n";
            else if (r == FR_NOW_FRIENDS) { std::cout << "  " << otherName << " had already asked you, you are friends now!\n"; changed = true; }
            else { std::cout << "  Request sent to " << otherName << ".\n"; changed = true; }
        } else if (c == 2) {
            if (acceptRequest(st.friends, me, otherId)) { std::cout << "  You are now friends with " << otherName << ".\n"; changed = true; }
            else std::cout << "  " << otherName << " has not sent you a request.\n";
        } else {
            if (removeLink(st.friends, me, otherId)) { std::cout << "  Removed " << otherName << ".\n"; changed = true; }
            else std::cout << "  Nothing to remove for " << otherName << ".\n";
        }
        if (changed) tx.commit();
    }
}

//==========================
// chat
//==========================
// moves the "read up to" marker forward, but only when nothing else is still unread
void markChatRead(Store& st) {
    if (st.chat.totalUnread() != 0) return;
    int newest = st.chat.latestMessageId();
    if (newest <= st.current->lastSeenMsg) return;

    Transaction tx(st);
    if (!tx.ok()) return;
    if (newest > st.current->lastSeenMsg) st.current->lastSeenMsg = newest;
    tx.commit();
}

void runChat(Store& st, int friendId, const std::string& friendName) {
    int me = st.current->userId;
    std::string myName = st.current->username;

    st.chat.setOpenPartner(friendId);          // from now on the worker thread prints this friend's messages live
    st.chat.clearUnread(friendId);

    std::cout << "\n===== Chat with " << friendName << " =====\n(type /exit to leave the chat)\n";
    std::vector<ChatMessage> history = loadConversation(CHAT_FILE, me, friendId, 15);
    if (history.empty()) std::cout << "  (no messages yet, say hi!)\n";
    for (size_t i = 0; i < history.size(); i++) printChatMessage(history[i]);

    while (true) {
        std::string line;
        {
            std::lock_guard<std::mutex> guard(consoleMutex());
            std::cout << "You: " << std::flush;
        }
        if (!std::getline(std::cin, line)) break;
        line = gameTrim(line);
        if (line == "/exit") break;
        if (line.empty()) continue;

        if (sendChatMessage(CHAT_FILE, me, myName, friendId, line) == 0)
            std::cout << "  (message could not be sent)\n";
    }

    st.chat.setOpenPartner(0);
    st.chat.clearUnread(friendId);
    markChatRead(st);
}

void doMessages(Store& st) {
    while (true) {
        reloadStore(st);
        showFriends(st);
        std::cout << "1. Open a chat\n2. Mark all messages as read\n0. Back\n";
        int c = readInt("Choice: ");
        if (c == -2 || c == 0) return;

        if (c == 1) {
            std::string name;
            if (!readLine("Chat with (username): ", name)) return;
            AccountNode* other = findAccountByName(st.accounts, gameTrim(name));
            if (other == nullptr || !areFriends(st.friends, st.current->userId, other->userId)) {
                std::cout << "  You can only chat with your friends.\n";
                continue;
            }
            runChat(st, other->userId, other->username);
        } else if (c == 2) {
            st.chat.clearAllUnread();
            markChatRead(st);
            std::cout << "  All messages marked as read.\n";
        } else {
            std::cout << "  Invalid choice.\n";
        }
    }
}

//==========================
// menus
//==========================
// returns false when the user chooses Exit (or the input ended)
bool guestMenu(Store& st) {
    reloadStore(st);
    showStorePage(st);
    std::cout << "\n1. Create account\n2. Log in\n3. View a game page\n4. Search\n0. Exit\n";
    int c = readInt("Choice: ");
    if (c == -2) return false;              // input ended, leave the program
    switch (c) {
        case 1: doRegister(st); break;
        case 2: doLogin(st); break;
        case 3: doViewGame(st); break;
        case 4: doSearch(st); break;
        case 0: return false;
        default: std::cout << "  Invalid choice.\n";
    }
    return true;
}

bool userMenu(Store& st) {
    reloadStore(st);
    if (st.current == nullptr) { doLogout(st); return true; }      // account vanished (should not happen)
    showStorePage(st);
    std::cout << "\n1. View a game page\n2. Search\n3. Add a game to the cart\n4. Cart / checkout\n"
              << "5. Wishlist\n6. My library\n7. Post a review / comment\n8. Publish a new game\n"
              << "9. Wallet (add funds)\n10. Friends\n11. Messages\n12. Log out\n0. Exit\n";
    int c = readInt("Choice: ");
    if (c == -2) return false;              // input ended, leave the program
    switch (c) {
        case 1: doViewGame(st); break;
        case 2: doSearch(st); break;
        case 3: doAddToCart(st); break;
        case 4: doCart(st); break;
        case 5: doWishlist(st); break;
        case 6: doLibrary(st); break;
        case 7: doReview(st); break;
        case 8: doPublish(st); break;
        case 9: doWallet(st); break;
        case 10: doFriends(st); break;
        case 11: doMessages(st); break;
        case 12: doLogout(st); break;
        case 0: return false;
        default: std::cout << "  Invalid choice.\n";
    }
    return true;
}

//==========================
// entry point of the whole platform
//==========================
void runStore() {
    Store st;
    initStore(st);
    std::cout << "=== Welcome to the Game Store ===\n";

    bool running = true;
    while (running) {
        running = (st.current == nullptr) ? guestMenu(st) : userMenu(st);
    }

    st.chat.stop();                         // make sure the background thread is finished
    clearCart(st.cart);
    freeStoreData(st);
    std::cout << "Goodbye!\n";
}

#endif //STEAM_STORE_H
