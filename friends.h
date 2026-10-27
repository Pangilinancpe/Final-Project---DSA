#ifndef STEAM_FRIENDS_H
#define STEAM_FRIENDS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

//==========================================
// FRIENDS MODULE
// A friendship is one record between two user ids:
//   fromId -> toId, accepted = false  : request still waiting
//   fromId -> toId, accepted = true   : they are friends
// Storage: singly linked list.   File: steam_friends.txt -> from|to|1or0
// Only ids are used here, so this module does not need account.h.
//==========================================

struct FriendNode {
    int fromId = 0;
    int toId = 0;
    bool accepted = false;
    FriendNode* next = nullptr;
};

struct FriendList {
    FriendNode* head = nullptr;
    FriendNode* tail = nullptr;
    int count = 0;
};

// result codes of sendFriendRequest
const int FR_SENT        = 1;    // new request created
const int FR_NOW_FRIENDS = 2;    // the other user had already asked us, so both are friends now
const int FR_SELF        = -1;   // cannot add yourself
const int FR_ALREADY     = -2;   // already friends
const int FR_PENDING     = -3;   // we already sent a request

// the record between two users, in either direction (nullptr if none)
FriendNode* findLink(const FriendList& list, int a, int b) {
    for (FriendNode* cur = list.head; cur != nullptr; cur = cur->next) {
        if ((cur->fromId == a && cur->toId == b) || (cur->fromId == b && cur->toId == a)) return cur;
    }
    return nullptr;
}

bool areFriends(const FriendList& list, int a, int b) {
    FriendNode* link = findLink(list, a, b);
    return link != nullptr && link->accepted;
}

int sendFriendRequest(FriendList& list, int me, int other) {
    if (me == other) return FR_SELF;

    FriendNode* link = findLink(list, me, other);
    if (link != nullptr) {
        if (link->accepted) return FR_ALREADY;
        if (link->fromId == me) return FR_PENDING;
        link->accepted = true;                  // they asked first, so just accept
        return FR_NOW_FRIENDS;
    }

    FriendNode* node = new FriendNode();
    node->fromId = me;
    node->toId = other;
    if (list.head == nullptr) list.head = node;
    else list.tail->next = node;
    list.tail = node;
    list.count++;
    return FR_SENT;
}

// accept a request that "from" sent to "me"
bool acceptRequest(FriendList& list, int me, int from) {
    for (FriendNode* cur = list.head; cur != nullptr; cur = cur->next) {
        if (cur->fromId == from && cur->toId == me && !cur->accepted) {
            cur->accepted = true;
            return true;
        }
    }
    return false;
}

// removes the record between two users: unfriend, decline, or cancel
bool removeLink(FriendList& list, int a, int b) {
    FriendNode* prev = nullptr;
    for (FriendNode* cur = list.head; cur != nullptr; prev = cur, cur = cur->next) {
        bool match = (cur->fromId == a && cur->toId == b) || (cur->fromId == b && cur->toId == a);
        if (!match) continue;
        if (prev == nullptr) list.head = cur->next;
        else prev->next = cur->next;
        if (cur == list.tail) list.tail = prev;
        delete cur;
        list.count--;
        return true;
    }
    return false;
}

// how many requests are waiting for this user
int countIncoming(const FriendList& list, int me) {
    int n = 0;
    for (FriendNode* cur = list.head; cur != nullptr; cur = cur->next) {
        if (cur->toId == me && !cur->accepted) n++;
    }
    return n;
}

bool saveFriends(const FriendList& list, const std::string& file) {
    std::ofstream out(file.c_str());
    if (!out) return false;
    for (FriendNode* cur = list.head; cur != nullptr; cur = cur->next) {
        out << cur->fromId << '|' << cur->toId << '|' << (cur->accepted ? 1 : 0) << '\n';
    }
    return true;
}

bool loadFriends(FriendList& list, const std::string& file) {
    std::ifstream in(file.c_str());
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string a, b, c;
        std::getline(ss, a, '|');
        std::getline(ss, b, '|');
        std::getline(ss, c);
        if (a.empty() || b.empty()) continue;

        FriendNode* node = new FriendNode();
        node->fromId = std::atoi(a.c_str());
        node->toId = std::atoi(b.c_str());
        node->accepted = (c.find('1') != std::string::npos);
        if (list.head == nullptr) list.head = node;
        else list.tail->next = node;
        list.tail = node;
        list.count++;
    }
    return true;
}

void freeFriends(FriendList& list) {
    FriendNode* cur = list.head;
    while (cur != nullptr) {
        FriendNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    list.head = list.tail = nullptr;
    list.count = 0;
}

#endif //STEAM_FRIENDS_H
