#ifndef STEAM_CART_H
#define STEAM_CART_H

//==========================================
// CART MODULE
// A shopping cart is a singly linked list of game ids.
// It only lives during the login session (it is not saved to a file),
// like a real store cart that you fill before checkout.
//==========================================

struct CartNode {
    int gameId = 0;
    CartNode* next = nullptr;
};

struct Cart {
    CartNode* head = nullptr;
    int count = 0;
};

bool inCart(const Cart& cart, int gameId) {
    for (CartNode* cur = cart.head; cur != nullptr; cur = cur->next) {
        if (cur->gameId == gameId) return true;
    }
    return false;
}

// returns false when the game is already in the cart
bool addToCart(Cart& cart, int gameId) {
    if (inCart(cart, gameId)) return false;
    CartNode* node = new CartNode();
    node->gameId = gameId;
    if (cart.head == nullptr) {
        cart.head = node;
    } else {
        CartNode* cur = cart.head;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = node;
    }
    cart.count++;
    return true;
}

bool removeFromCart(Cart& cart, int gameId) {
    CartNode* prev = nullptr;
    for (CartNode* cur = cart.head; cur != nullptr; prev = cur, cur = cur->next) {
        if (cur->gameId != gameId) continue;
        if (prev == nullptr) cart.head = cur->next;
        else prev->next = cur->next;
        delete cur;
        cart.count--;
        return true;
    }
    return false;
}

void clearCart(Cart& cart) {
    CartNode* cur = cart.head;
    while (cur != nullptr) {
        CartNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    cart.head = nullptr;
    cart.count = 0;
}

#endif //STEAM_CART_H
