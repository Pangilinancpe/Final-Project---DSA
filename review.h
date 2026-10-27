#ifndef STEAM_REVIEW_H
#define STEAM_REVIEW_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

//==========================================
// REVIEW MODULE (comments + recommendations)
// Each review has: review id, game id, user id, username,
//                  recommended (yes / no), comment.
// Storage: singly linked list in the order they were posted
// Rule   : one review per user per game, posting again UPDATES it
// File   : steam_reviews.txt -> id|gameId|userId|username|1or0|comment
// This module only knows ids, so it does not need game.h or account.h.
//==========================================

struct ReviewNode {
    int reviewId = 0;
    int gameId = 0;
    int userId = 0;
    std::string username;
    bool recommended = false;
    std::string comment;
    ReviewNode* next = nullptr;
};

struct ReviewList {
    ReviewNode* head = nullptr;
    ReviewNode* tail = nullptr;
    int nextId = 1;
    int count = 0;
};

// '|' and line breaks would break the file format
std::string reviewClean(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '|' || s[i] == '\n' || s[i] == '\r') s[i] = '/';
    }
    return s;
}

//==========================
// find a user's review of a game (nullptr if none)
//==========================
ReviewNode* findReview(const ReviewList& list, int gameId, int userId) {
    for (ReviewNode* cur = list.head; cur != nullptr; cur = cur->next) {
        if (cur->gameId == gameId && cur->userId == userId) return cur;
    }
    return nullptr;
}

//==========================
// post a review
// returns 1 = new review, 2 = existing review updated, 0 = empty comment (rejected)
//==========================
int postReview(ReviewList& list, int gameId, int userId, const std::string& username,
               bool recommended, std::string comment) {
    comment = reviewClean(comment);
    if (comment.find_first_not_of(" \t") == std::string::npos) return 0;

    ReviewNode* existing = findReview(list, gameId, userId);
    if (existing != nullptr) {
        existing->recommended = recommended;
        existing->comment = comment;
        return 2;
    }

    ReviewNode* node = new ReviewNode();
    node->reviewId = list.nextId++;
    node->gameId = gameId;
    node->userId = userId;
    node->username = username;
    node->recommended = recommended;
    node->comment = comment;

    if (list.head == nullptr) list.head = node;
    else list.tail->next = node;
    list.tail = node;
    list.count++;
    return 1;
}

//==========================
// recommendation stats of one game
//==========================
void reviewStats(const ReviewList& list, int gameId, int& total, int& recommended) {
    total = 0;
    recommended = 0;
    for (ReviewNode* cur = list.head; cur != nullptr; cur = cur->next) {
        if (cur->gameId != gameId) continue;
        total++;
        if (cur->recommended) recommended++;
    }
}

// percentage recommended, -1 when the game has no reviews yet
int recommendPercent(const ReviewList& list, int gameId) {
    int total, rec;
    reviewStats(list, gameId, total, rec);
    if (total == 0) return -1;
    return (rec * 100) / total;
}

//==========================
// show all reviews of a game
//==========================
void displayReviewsForGame(const ReviewList& list, int gameId) {
    int shown = 0;
    for (ReviewNode* cur = list.head; cur != nullptr; cur = cur->next) {
        if (cur->gameId != gameId) continue;
        std::cout << "    " << (cur->recommended ? "[+ Recommended]     " : "[- Not Recommended] ")
                  << cur->username << ": " << cur->comment << "\n";
        shown++;
    }
    if (shown == 0) std::cout << "    (no reviews yet)\n";
}

//==========================
// file saving / loading
//==========================
bool saveReviews(const ReviewList& list, const std::string& file) {
    std::ofstream out(file.c_str());
    if (!out) return false;
    for (ReviewNode* cur = list.head; cur != nullptr; cur = cur->next) {
        out << cur->reviewId << '|' << cur->gameId << '|' << cur->userId << '|'
            << cur->username << '|' << (cur->recommended ? 1 : 0) << '|' << cur->comment << '\n';
    }
    return true;
}

bool loadReviews(ReviewList& list, const std::string& file) {
    std::ifstream in(file.c_str());
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string idText, gameText, userText, name, recText, comment;
        std::getline(ss, idText, '|');
        std::getline(ss, gameText, '|');
        std::getline(ss, userText, '|');
        std::getline(ss, name, '|');
        std::getline(ss, recText, '|');
        std::getline(ss, comment);
        if (idText.empty() || gameText.empty() || userText.empty()) continue;

        ReviewNode* node = new ReviewNode();
        node->reviewId = std::atoi(idText.c_str());
        node->gameId = std::atoi(gameText.c_str());
        node->userId = std::atoi(userText.c_str());
        node->username = name;
        node->recommended = (recText == "1");
        node->comment = comment;

        if (list.head == nullptr) list.head = node;
        else list.tail->next = node;
        list.tail = node;
        list.count++;
        if (node->reviewId >= list.nextId) list.nextId = node->reviewId + 1;
    }
    return true;
}

void freeReviews(ReviewList& list) {
    ReviewNode* cur = list.head;
    while (cur != nullptr) {
        ReviewNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    list.head = list.tail = nullptr;
    list.count = 0;
    list.nextId = 1;
}

#endif //STEAM_REVIEW_H
