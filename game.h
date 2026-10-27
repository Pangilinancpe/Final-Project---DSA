#ifndef STEAM_GAME_H
#define STEAM_GAME_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

//==========================================
// GAME MODULE (game creation)
// Each game has: title, game id, description, tags.
// Storage: singly linked list (keeps the insertion / id order)
// Search : BST keyed by lowercase title (fast title lookup)
// File   : steam_games.txt  ->  id|title|description|tags|priceCents|developerId
// Tags   : stored as lowercase, comma separated text, e.g. "action, co-op" (max 8 per game)
//==========================================

//--------------------------
// linked list node of one game
//--------------------------
struct GameNode {
    int gameId = 0;
    std::string title;
    std::string description;
    std::string tags;          // comma separated, e.g. "action, co-op"
    int priceCents = 0;        // price in cents (1499 = $14.99), 0 = free
    int developerId = 0;       // user id of the developer who published it, 0 = the platform itself
    GameNode* next = nullptr;
};

//--------------------------
// BST node, points to the game in the list (no copy of the data)
//--------------------------
struct GameBSTNode {
    GameNode* game = nullptr;
    GameBSTNode* left = nullptr;
    GameBSTNode* right = nullptr;
};

//--------------------------
// the whole catalog = list + BST + counters
//--------------------------
struct GameCatalog {
    GameNode* head = nullptr;
    GameNode* tail = nullptr;
    GameBSTNode* root = nullptr;
    int nextId = 1;
    int count = 0;
};

//==========================
// small string helpers
//==========================
std::string gameToLower(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = s[i] - 'A' + 'a';
    }
    return s;
}

// the file uses '|' as separator, so it can not appear inside the text
std::string gameClean(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '|' || s[i] == '\n' || s[i] == '\r') s[i] = '/';
    }
    return s;
}

// 1499 -> "$14.99"
std::string moneyText(int cents) {
    int dollars = cents / 100;
    int rest = cents % 100;
    if (rest < 0) rest = -rest;
    return "$" + std::to_string(dollars) + "." + (rest < 10 ? "0" : "") + std::to_string(rest);
}

// 0 -> "Free", otherwise the amount
std::string priceText(int cents) {
    return cents <= 0 ? std::string("Free") : moneyText(cents);
}

std::string gameTrim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

//==========================
// tags
//==========================
const size_t MAX_TAGS = 8;           // tags per game
const size_t MAX_TAG_LENGTH = 20;    // characters per tag

// "Action,  CO-OP ,action" -> {"action", "co-op"}   (lowercase, trimmed, no empty, no duplicates)
std::vector<std::string> splitTags(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string part;
    while (std::getline(ss, part, ',')) {
        part = gameToLower(gameTrim(part));
        if (part.empty()) continue;
        if (part.size() > MAX_TAG_LENGTH) part.resize(MAX_TAG_LENGTH);
        bool duplicate = false;
        for (size_t i = 0; i < out.size(); i++) if (out[i] == part) duplicate = true;
        if (!duplicate) out.push_back(part);
    }
    if (out.size() > MAX_TAGS) out.resize(MAX_TAGS);
    return out;
}

// cleaned text that is stored in the file: "action, co-op"
std::string normalizeTags(const std::string& text) {
    std::vector<std::string> parts = splitTags(text);
    std::string out;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) out += ", ";
        out += parts[i];
    }
    return out;
}

// exact tag match (not just a part of a word)
bool gameHasTag(const GameNode* g, const std::string& tag) {
    std::string wanted = gameToLower(gameTrim(tag));
    std::vector<std::string> own = splitTags(g->tags);
    for (size_t i = 0; i < own.size(); i++) if (own[i] == wanted) return true;
    return false;
}

//==========================
// BST (title -> game)
//==========================
void gameBstInsert(GameBSTNode*& root, GameNode* game) {
    if (root == nullptr) {
        root = new GameBSTNode();
        root->game = game;
        return;
    }
    if (gameToLower(game->title) < gameToLower(root->game->title))
        gameBstInsert(root->left, game);
    else
        gameBstInsert(root->right, game);
}

GameNode* gameBstFind(GameBSTNode* root, const std::string& lowerTitle) {
    while (root != nullptr) {
        std::string key = gameToLower(root->game->title);
        if (lowerTitle == key) return root->game;
        root = (lowerTitle < key) ? root->left : root->right;
    }
    return nullptr;
}

void gameBstFree(GameBSTNode* root) {
    if (root == nullptr) return;
    gameBstFree(root->left);
    gameBstFree(root->right);
    delete root;
}

//==========================
// create / add a game
// returns nullptr when the title is empty or already used
//==========================
GameNode* createGame(GameCatalog& cat, std::string title, std::string description, std::string tags,
                     int priceCents = 0, int developerId = 0) {
    title = gameClean(gameTrim(title));
    description = gameClean(gameTrim(description));
    tags = normalizeTags(gameClean(tags));

    if (title.empty()) return nullptr;
    if (gameBstFind(cat.root, gameToLower(title)) != nullptr) return nullptr;   // duplicate title

    GameNode* node = new GameNode();
    node->gameId = cat.nextId++;
    node->title = title;
    node->description = description;
    node->tags = tags;
    node->priceCents = priceCents < 0 ? 0 : priceCents;
    node->developerId = developerId;

    if (cat.head == nullptr) cat.head = node;
    else cat.tail->next = node;
    cat.tail = node;
    cat.count++;

    gameBstInsert(cat.root, node);
    return node;
}

//==========================
// edit a game (title and id never change). empty text / negative price = keep the old value
//==========================
void updateGame(GameNode* g, const std::string& description, const std::string& tags, int priceCents) {
    std::string d = gameClean(gameTrim(description));
    std::string t = normalizeTags(gameClean(tags));
    if (!d.empty()) g->description = d;
    if (!t.empty()) g->tags = t;
    if (priceCents >= 0) g->priceCents = priceCents;
}

//==========================
// searching
//==========================
// by id: walks the list (ids grow 1,2,3.. so a BST by id would only become a chain)
GameNode* findGameById(const GameCatalog& cat, int id) {
    for (GameNode* cur = cat.head; cur != nullptr; cur = cur->next) {
        if (cur->gameId == id) return cur;
    }
    return nullptr;
}

// by exact title (case-insensitive) through the BST
GameNode* findGameByTitle(const GameCatalog& cat, const std::string& title) {
    return gameBstFind(cat.root, gameToLower(gameTrim(title)));
}

// true when the keyword is inside the title or any tag (case-insensitive)
bool gameMatches(const GameNode* g, const std::string& keyword) {
    std::string k = gameToLower(keyword);
    return gameToLower(g->title).find(k) != std::string::npos ||
           gameToLower(g->tags).find(k) != std::string::npos;
}

//==========================
// display
//==========================
void displayGameDetails(const GameNode* g) {
    std::cout << "  [" << g->gameId << "] " << g->title << "\n"
              << "      " << g->description << "\n"
              << "      Tags: " << (g->tags.empty() ? "(none)" : g->tags) << "\n"
              << "      Price: " << priceText(g->priceCents) << "\n";
}

//==========================
// file saving / loading
//==========================
bool saveGames(const GameCatalog& cat, const std::string& file) {
    std::ofstream out(file.c_str());
    if (!out) return false;
    for (GameNode* cur = cat.head; cur != nullptr; cur = cur->next) {
        out << cur->gameId << '|' << cur->title << '|' << cur->description << '|' << cur->tags << '|' << cur->priceCents << '|' << cur->developerId << '\n';
    }
    return true;
}

// a missing file is fine (first run) -> returns false and the catalog stays empty
bool loadGames(GameCatalog& cat, const std::string& file) {
    std::ifstream in(file.c_str());
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string idText, title, desc, tags, priceField, devField;
        std::getline(ss, idText, '|');
        std::getline(ss, title, '|');
        std::getline(ss, desc, '|');
        std::getline(ss, tags, '|');
        std::getline(ss, priceField, '|');            // older files have no price -> free
        std::getline(ss, devField);                   // older files have no developer -> the platform
        if (idText.empty() || title.empty()) continue;      // skip broken lines

        GameNode* node = new GameNode();
        node->gameId = std::atoi(idText.c_str());
        node->title = title;
        node->description = desc;
        node->tags = normalizeTags(tags);
        node->developerId = std::atoi(devField.c_str());
        node->priceCents = std::atoi(priceField.c_str());
        if (node->priceCents < 0) node->priceCents = 0;

        if (cat.head == nullptr) cat.head = node;
        else cat.tail->next = node;
        cat.tail = node;
        cat.count++;
        if (node->gameId >= cat.nextId) cat.nextId = node->gameId + 1;
        gameBstInsert(cat.root, node);
    }
    return true;
}

//==========================
// free everything
//==========================
void freeCatalog(GameCatalog& cat) {
    GameNode* cur = cat.head;
    while (cur != nullptr) {
        GameNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    gameBstFree(cat.root);
    cat.head = cat.tail = nullptr;
    cat.root = nullptr;
    cat.count = 0;
    cat.nextId = 1;
}

#endif //STEAM_GAME_H
