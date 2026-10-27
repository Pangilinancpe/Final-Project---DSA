#ifndef STEAM_ACCOUNT_H
#define STEAM_ACCOUNT_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

//==========================================
// ACCOUNT MODULE (account creation)
// Each account has: username, user id, password, games owned.
// Storage: singly linked list of accounts, each with its own linked
//          list of owned game ids
// Search : BST keyed by lowercase username (login / duplicate check)
// File   : steam_accounts.txt -> id|username|passwordHash|ownedIds|walletCents|wishlistIds|lastSeenMsg|role
//          (id lists are comma separated, e.g. 1,3,4)
// Role   : 0 = player (normal user), 1 = developer (can publish and edit games)
// Extra per account: wallet balance (in cents), wishlist (linked list of game ids)
//          and the newest chat message the user has already read.
// NOTE   : the password is stored as a simple djb2 hash, not plain text.
//          It is good enough for a class project, NOT real security.
//==========================================

//--------------------------
// one owned game id in an account's library
//--------------------------
struct OwnedNode {
    int gameId = 0;
    OwnedNode* next = nullptr;
};

//--------------------------
// one wishlist entry
//--------------------------
struct WishNode {
    int gameId = 0;
    WishNode* next = nullptr;
};

// account types
const int ROLE_PLAYER = 0;
const int ROLE_DEVELOPER = 1;

// the wallet can not grow past $9,999.99 (keeps the cents inside an int)
const int MAX_WALLET_CENTS = 999999;

//--------------------------
// one account
//--------------------------
struct AccountNode {
    int userId = 0;
    std::string username;
    unsigned long long passwordHash = 0;
    OwnedNode* owned = nullptr;
    int ownedCount = 0;
    int walletCents = 0;
    WishNode* wishlist = nullptr;
    int wishCount = 0;
    int lastSeenMsg = 0;            // newest chat message id the user has read
    int role = ROLE_PLAYER;
    AccountNode* next = nullptr;
};

struct AccountBSTNode {
    AccountNode* account = nullptr;
    AccountBSTNode* left = nullptr;
    AccountBSTNode* right = nullptr;
};

struct AccountRegistry {
    AccountNode* head = nullptr;
    AccountNode* tail = nullptr;
    AccountBSTNode* root = nullptr;
    int nextId = 1;
    int count = 0;
};

//==========================
// helpers
//==========================
std::string accToLower(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = s[i] - 'A' + 'a';
    }
    return s;
}

// djb2 string hash
unsigned long long hashPassword(const std::string& pw) {
    unsigned long long h = 5381;
    for (size_t i = 0; i < pw.size(); i++) {
        h = ((h << 5) + h) + (unsigned char)pw[i];
    }
    return h;
}

// username rule: 3-20 characters, letters / digits / underscore only
bool validUsername(const std::string& u) {
    if (u.size() < 3 || u.size() > 20) return false;
    for (size_t i = 0; i < u.size(); i++) {
        char c = u[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

//==========================
// BST (username -> account)
//==========================
void accBstInsert(AccountBSTNode*& root, AccountNode* acc) {
    if (root == nullptr) {
        root = new AccountBSTNode();
        root->account = acc;
        return;
    }
    if (accToLower(acc->username) < accToLower(root->account->username))
        accBstInsert(root->left, acc);
    else
        accBstInsert(root->right, acc);
}

AccountNode* accBstFind(AccountBSTNode* root, const std::string& lowerName) {
    while (root != nullptr) {
        std::string key = accToLower(root->account->username);
        if (lowerName == key) return root->account;
        root = (lowerName < key) ? root->left : root->right;
    }
    return nullptr;
}

void accBstFree(AccountBSTNode* root) {
    if (root == nullptr) return;
    accBstFree(root->left);
    accBstFree(root->right);
    delete root;
}

//==========================
// find an account
//==========================
AccountNode* findAccountByName(const AccountRegistry& reg, const std::string& username) {
    return accBstFind(reg.root, accToLower(username));
}

AccountNode* findAccountById(const AccountRegistry& reg, int id) {
    for (AccountNode* cur = reg.head; cur != nullptr; cur = cur->next) {
        if (cur->userId == id) return cur;
    }
    return nullptr;
}

//==========================
// account creation
// returns nullptr if the username is invalid / taken or the password is under 4 chars
//==========================
AccountNode* createAccount(AccountRegistry& reg, const std::string& username, const std::string& password,
                           int role = ROLE_PLAYER) {
    if (!validUsername(username)) return nullptr;
    if (password.size() < 4) return nullptr;
    if (findAccountByName(reg, username) != nullptr) return nullptr;

    AccountNode* node = new AccountNode();
    node->userId = reg.nextId++;
    node->username = username;
    node->passwordHash = hashPassword(password);
    node->role = (role == ROLE_DEVELOPER) ? ROLE_DEVELOPER : ROLE_PLAYER;

    if (reg.head == nullptr) reg.head = node;
    else reg.tail->next = node;
    reg.tail = node;
    reg.count++;

    accBstInsert(reg.root, node);
    return node;
}

//==========================
// login: returns the account when username and password match, else nullptr
//==========================
AccountNode* loginAccount(const AccountRegistry& reg, const std::string& username, const std::string& password) {
    AccountNode* acc = findAccountByName(reg, username);
    if (acc == nullptr) return nullptr;
    if (acc->passwordHash != hashPassword(password)) return nullptr;
    return acc;
}

bool isDeveloper(const AccountNode* acc) {
    return acc != nullptr && acc->role == ROLE_DEVELOPER;
}

//==========================
// owned games
//==========================
bool ownsGame(const AccountNode* acc, int gameId) {
    for (OwnedNode* cur = acc->owned; cur != nullptr; cur = cur->next) {
        if (cur->gameId == gameId) return true;
    }
    return false;
}

// returns false when the game is already in the library
bool addOwnedGame(AccountNode* acc, int gameId) {
    if (ownsGame(acc, gameId)) return false;

    OwnedNode* node = new OwnedNode();
    node->gameId = gameId;
    if (acc->owned == nullptr) {
        acc->owned = node;
    } else {
        OwnedNode* cur = acc->owned;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = node;
    }
    acc->ownedCount++;
    return true;
}

//==========================
// wallet (simulated money, no real payment is involved)
//==========================
// returns false when the new balance would pass MAX_WALLET_CENTS
bool addFunds(AccountNode* acc, int cents) {
    if (cents <= 0 || acc->walletCents + cents > MAX_WALLET_CENTS) return false;
    acc->walletCents += cents;
    return true;
}

// returns false when the balance is not enough
bool spendFunds(AccountNode* acc, int cents) {
    if (cents < 0 || cents > acc->walletCents) return false;
    acc->walletCents -= cents;
    return true;
}

//==========================
// wishlist
//==========================
bool onWishlist(const AccountNode* acc, int gameId) {
    for (WishNode* cur = acc->wishlist; cur != nullptr; cur = cur->next) {
        if (cur->gameId == gameId) return true;
    }
    return false;
}

// returns false when the game is already on the wishlist
bool addWish(AccountNode* acc, int gameId) {
    if (onWishlist(acc, gameId)) return false;
    WishNode* node = new WishNode();
    node->gameId = gameId;
    if (acc->wishlist == nullptr) {
        acc->wishlist = node;
    } else {
        WishNode* cur = acc->wishlist;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = node;
    }
    acc->wishCount++;
    return true;
}

bool removeWish(AccountNode* acc, int gameId) {
    WishNode* prev = nullptr;
    for (WishNode* cur = acc->wishlist; cur != nullptr; prev = cur, cur = cur->next) {
        if (cur->gameId != gameId) continue;
        if (prev == nullptr) acc->wishlist = cur->next;
        else prev->next = cur->next;
        delete cur;
        acc->wishCount--;
        return true;
    }
    return false;
}

//==========================
// file saving / loading
//==========================
bool saveAccounts(const AccountRegistry& reg, const std::string& file) {
    std::ofstream out(file.c_str());
    if (!out) return false;
    for (AccountNode* cur = reg.head; cur != nullptr; cur = cur->next) {
        out << cur->userId << '|' << cur->username << '|' << cur->passwordHash << '|';
        for (OwnedNode* o = cur->owned; o != nullptr; o = o->next) {
            out << o->gameId;
            if (o->next != nullptr) out << ',';
        }
        out << '|' << cur->walletCents << '|';
        for (WishNode* w = cur->wishlist; w != nullptr; w = w->next) {
            out << w->gameId;
            if (w->next != nullptr) out << ',';
        }
        out << '|' << cur->lastSeenMsg << '|' << cur->role << '\n';
    }
    return true;
}

bool loadAccounts(AccountRegistry& reg, const std::string& file) {
    std::ifstream in(file.c_str());
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string idText, name, hashText, ownedText, walletText, wishText, seenText, roleText;
        std::getline(ss, idText, '|');
        std::getline(ss, name, '|');
        std::getline(ss, hashText, '|');
        std::getline(ss, ownedText, '|');
        std::getline(ss, walletText, '|');       // older files stop after the owned ids,
        std::getline(ss, wishText, '|');         // the missing fields simply stay empty / 0
        std::getline(ss, seenText, '|');
        std::getline(ss, roleText);              // older files have no role -> player
        if (idText.empty() || name.empty() || hashText.empty()) continue;

        AccountNode* node = new AccountNode();
        node->userId = std::atoi(idText.c_str());
        node->username = name;
        node->passwordHash = std::strtoull(hashText.c_str(), nullptr, 10);

        // owned ids "1,3,4"
        std::stringstream os(ownedText);
        std::string part;
        while (std::getline(os, part, ',')) {
            if (!part.empty()) addOwnedGame(node, std::atoi(part.c_str()));
        }

        node->walletCents = std::atoi(walletText.c_str());
        if (node->walletCents < 0) node->walletCents = 0;
        std::stringstream ws(wishText);
        while (std::getline(ws, part, ',')) {
            if (!part.empty()) addWish(node, std::atoi(part.c_str()));
        }
        node->lastSeenMsg = std::atoi(seenText.c_str());
        node->role = (std::atoi(roleText.c_str()) == ROLE_DEVELOPER) ? ROLE_DEVELOPER : ROLE_PLAYER;

        if (reg.head == nullptr) reg.head = node;
        else reg.tail->next = node;
        reg.tail = node;
        reg.count++;
        if (node->userId >= reg.nextId) reg.nextId = node->userId + 1;
        accBstInsert(reg.root, node);
    }
    return true;
}

//==========================
// free everything
//==========================
void freeAccounts(AccountRegistry& reg) {
    AccountNode* cur = reg.head;
    while (cur != nullptr) {
        AccountNode* nxt = cur->next;
        OwnedNode* o = cur->owned;
        while (o != nullptr) {
            OwnedNode* on = o->next;
            delete o;
            o = on;
        }
        WishNode* w = cur->wishlist;
        while (w != nullptr) {
            WishNode* wn = w->next;
            delete w;
            w = wn;
        }
        delete cur;
        cur = nxt;
    }
    accBstFree(reg.root);
    reg.head = reg.tail = nullptr;
    reg.root = nullptr;
    reg.count = 0;
    reg.nextId = 1;
}

#endif //STEAM_ACCOUNT_H
