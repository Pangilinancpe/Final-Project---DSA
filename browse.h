#ifndef STEAM_BROWSE_H
#define STEAM_BROWSE_H

#include <string>
#include <vector>
#include <map>
#include "game.h"
#include "review.h"

//==========================================
// BROWSE MODULE (find games by tag, sort the list)
//
// collectGames        : filters the catalog by tags (a game must have ALL wanted tags)
// buildTagIndex       : every tag in the store with the number of games that use it
// insertionSortGames  : sorts the filtered list (stable insertion sort, from HOA 8.1)
// The functions work on lists of pointers, the catalog itself is never reordered.
//==========================================

enum SortKey {
    SORT_ID = 0,        // order they were added
    SORT_TITLE,         // A to Z
    SORT_PRICE_LOW,     // cheapest first
    SORT_PRICE_HIGH,    // most expensive first
    SORT_RATING,        // highest recommendation percent first
    SORT_KEY_COUNT
};

std::string sortName(SortKey key) {
    switch (key) {
        case SORT_TITLE:      return "Title (A-Z)";
        case SORT_PRICE_LOW:  return "Price (low to high)";
        case SORT_PRICE_HIGH: return "Price (high to low)";
        case SORT_RATING:     return "Rating (best first)";
        default:              return "Newest last (default)";
    }
}

// wanted tags must already be lowercase (use splitTags on the user's text)
bool gameHasAllTags(const GameNode* g, const std::vector<std::string>& wanted) {
    std::vector<std::string> own = splitTags(g->tags);
    for (size_t w = 0; w < wanted.size(); w++) {
        bool found = false;
        for (size_t o = 0; o < own.size(); o++) if (own[o] == wanted[w]) found = true;
        if (!found) return false;
    }
    return true;
}

// an empty tag list means "no filter"
std::vector<const GameNode*> collectGames(const GameCatalog& cat, const std::vector<std::string>& wantedTags) {
    std::vector<const GameNode*> out;
    for (GameNode* g = cat.head; g != nullptr; g = g->next) {
        if (gameHasAllTags(g, wantedTags)) out.push_back(g);
    }
    return out;
}

// tag -> how many games use it (std::map keeps the tags in alphabetical order)
std::map<std::string, int> buildTagIndex(const GameCatalog& cat) {
    std::map<std::string, int> index;
    for (GameNode* g = cat.head; g != nullptr; g = g->next) {
        std::vector<std::string> own = splitTags(g->tags);
        for (size_t i = 0; i < own.size(); i++) index[own[i]]++;
    }
    return index;
}

// true when a must be shown BEFORE b (strictly, so equal games keep their order)
bool goesBefore(const GameNode* a, const GameNode* b, SortKey key, const ReviewList& reviews) {
    switch (key) {
        case SORT_TITLE:      return gameToLower(a->title) < gameToLower(b->title);
        case SORT_PRICE_LOW:  return a->priceCents < b->priceCents;
        case SORT_PRICE_HIGH: return a->priceCents > b->priceCents;
        case SORT_RATING:     return recommendPercent(reviews, a->gameId) > recommendPercent(reviews, b->gameId);
        default:              return a->gameId < b->gameId;
    }
}

// insertion sort: take each game and slide it left until it is in place
void insertionSortGames(std::vector<const GameNode*>& list, SortKey key, const ReviewList& reviews) {
    for (size_t i = 1; i < list.size(); i++) {
        const GameNode* current = list[i];
        size_t j = i;
        while (j > 0 && goesBefore(current, list[j - 1], key, reviews)) {
            list[j] = list[j - 1];
            j--;
        }
        list[j] = current;
    }
}

#endif //STEAM_BROWSE_H
