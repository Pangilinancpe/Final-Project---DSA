#ifndef STEAM_CHAT_H
#define STEAM_CHAT_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include "sync.h"

//==========================================
// CHAT MODULE (messages between friends, with a background thread)
//
// Messages live in ONE shared text file, one line per message:
//   msgId|timestamp|fromId|fromName|toId|text
// Every terminal that runs the program reads and appends to that file,
// so two terminals on the same computer can chat with each other.
//
// THREADS:
//   main thread  -> menus, typing and sending messages
//   worker thread (ChatListener) -> every 300 ms checks the file for new
//                   messages addressed to the logged in user and shows them
//                   immediately, without the user pressing anything
// The worker never touches the accounts / games / reviews data. It only knows
// ids and names that come inside the message lines, so there is no data race
// on the store. The data the two threads DO share (unread counts, the open
// chat partner) is protected by a mutex / atomics, and printing is protected by
// consoleMutex() so lines do not get mixed.
//==========================================

const size_t MAX_CHAT_LENGTH = 300;

struct ChatMessage {
    int msgId = 0;
    long long ts = 0;
    int fromId = 0;
    std::string fromName;
    int toId = 0;
    std::string text;
};

// the file uses '|' as separator and one line per message
std::string chatClean(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '|' || s[i] == '\n' || s[i] == '\r') s[i] = '/';
    }
    return s;
}

// HH:MM of a timestamp. localtime() shares one buffer between callers, so it is
// guarded by its own mutex (both threads may format a time).
std::string chatTimeText(long long ts) {
    static std::mutex timeMutex;
    std::lock_guard<std::mutex> guard(timeMutex);
    std::time_t t = (std::time_t)ts;
    std::tm* parts = std::localtime(&t);
    if (parts == nullptr) return "--:--";
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M", parts);
    return std::string(buf);
}

bool parseChatLine(std::string line, ChatMessage& m) {
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);   // Windows line ending
    std::stringstream ss(line);
    std::string id, ts, from, name, to, text;
    std::getline(ss, id, '|');
    std::getline(ss, ts, '|');
    std::getline(ss, from, '|');
    std::getline(ss, name, '|');
    std::getline(ss, to, '|');
    std::getline(ss, text);
    if (id.empty() || ts.empty() || from.empty() || to.empty()) return false;

    m.msgId = std::atoi(id.c_str());
    m.ts = std::atoll(ts.c_str());
    m.fromId = std::atoi(from.c_str());
    m.fromName = name;
    m.toId = std::atoi(to.c_str());
    m.text = text;
    return m.msgId > 0 && m.fromId > 0 && m.toId > 0;
}

//==========================
// send: add one line to the file.
// The id is "highest id in the file + 1", so the file is locked while we do it
// (two terminals could send at the same moment).
// returns the new message id, or 0 when it failed
//==========================
int sendChatMessage(const std::string& file, int fromId, const std::string& fromName, int toId, std::string text) {
    text = chatClean(text);
    if (text.find_first_not_of(" \t") == std::string::npos) return 0;
    if (text.size() > MAX_CHAT_LENGTH) text.resize(MAX_CHAT_LENGTH);

    FileLock lock("steam_chat");
    if (!lock.locked()) return 0;

    int maxId = 0;
    {
        std::ifstream in(file.c_str());
        std::string line;
        ChatMessage m;
        while (std::getline(in, line)) {
            if (parseChatLine(line, m) && m.msgId > maxId) maxId = m.msgId;
        }
    }

    std::ofstream out(file.c_str(), std::ios::app);
    if (!out) return 0;
    int newId = maxId + 1;
    out << newId << '|' << (long long)std::time(nullptr) << '|' << fromId << '|' << fromName << '|'
        << toId << '|' << text << '\n';
    out.flush();
    return out ? newId : 0;
}

//==========================
// the last N messages between two users, oldest first
//==========================
std::vector<ChatMessage> loadConversation(const std::string& file, int a, int b, size_t lastN) {
    std::vector<ChatMessage> all;
    std::ifstream in(file.c_str());
    std::string line;
    ChatMessage m;
    while (std::getline(in, line)) {
        if (!parseChatLine(line, m)) continue;
        bool between = (m.fromId == a && m.toId == b) || (m.fromId == b && m.toId == a);
        if (between) all.push_back(m);
    }
    if (all.size() > lastN) all.erase(all.begin(), all.begin() + (all.size() - lastN));
    return all;
}

void printChatMessage(const ChatMessage& m) {
    std::cout << "  [" << chatTimeText(m.ts) << "] " << m.fromName << ": " << m.text << "\n";
}

//==========================================
// ChatListener: owns the worker thread
//==========================================
class ChatListener {
private:
    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<int> openPartner{0};      // friend whose chat is open right now (0 = none)
    std::atomic<int> latest{0};           // highest message id seen in the file

    mutable std::mutex dataMutex;         // guards the two maps below (mutable so const getters can lock it)
    std::map<int, int> unread;            // sender id -> number of unread messages
    std::map<int, std::string> senderNames;

    int myId = 0;
    std::string file;
    std::streamoff offset = 0;            // how far into the file we have already read

    //--------------------------
    // read the part of the file we have not seen yet.
    // initial = true : called once at login, counts messages that are newer than 'sinceId'
    //                  as unread, and prints nothing
    // initial = false: called by the worker, shows / counts new messages as they arrive
    //--------------------------
    void pollOnce(bool initial, int sinceId) {
        std::ifstream in(file.c_str(), std::ios::binary);
        if (!in) return;

        in.seekg(0, std::ios::end);
        std::streamoff size = in.tellg();
        if (size < offset) offset = 0;                 // file was replaced, start again
        if (size == offset) return;                    // nothing new

        in.seekg(offset);
        std::string chunk((size_t)(size - offset), '\0');
        in.read(&chunk[0], (std::streamsize)chunk.size());
        chunk.resize((size_t)in.gcount());

        // only handle complete lines (the sender may still be in the middle of writing one)
        size_t lastNewline = chunk.rfind('\n');
        if (lastNewline == std::string::npos) return;
        offset += (std::streamoff)(lastNewline + 1);

        std::stringstream lines(chunk.substr(0, lastNewline));
        std::string line;
        while (std::getline(lines, line)) {
            ChatMessage m;
            if (!parseChatLine(line, m)) continue;
            if (m.msgId > latest.load()) latest.store(m.msgId);
            if (m.toId != myId) continue;               // not for us

            if (initial) {
                if (m.msgId > sinceId) {
                    std::lock_guard<std::mutex> guard(dataMutex);
                    unread[m.fromId]++;
                    senderNames[m.fromId] = m.fromName;
                }
            } else if (openPartner.load() == m.fromId) {
                // the chat with this friend is open: show the message right away
                std::lock_guard<std::mutex> guard(consoleMutex());
                std::cout << "\n";
                printChatMessage(m);
                std::cout << "You: " << std::flush;
            } else {
                {
                    std::lock_guard<std::mutex> guard(dataMutex);
                    unread[m.fromId]++;
                    senderNames[m.fromId] = m.fromName;
                }
                std::lock_guard<std::mutex> guard(consoleMutex());
                std::cout << "\n  (!) New message from " << m.fromName << " - open Messages to read it.\n" << std::flush;
            }
        }
    }

    void workerLoop() {
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            if (!running.load()) break;
            pollOnce(false, 0);
        }
    }

public:
    ChatListener() {}
    ~ChatListener() { stop(); }

    ChatListener(const ChatListener&) = delete;
    ChatListener& operator=(const ChatListener&) = delete;

    // called at login. lastSeenId = newest message the user has already read
    void start(int userId, const std::string& chatFile, int lastSeenId) {
        stop();
        myId = userId;
        file = chatFile;
        offset = 0;
        latest.store(0);
        openPartner.store(0);
        {
            std::lock_guard<std::mutex> guard(dataMutex);
            unread.clear();
            senderNames.clear();
        }
        pollOnce(true, lastSeenId);        // counts the messages that arrived while offline
        running.store(true);
        worker = std::thread(&ChatListener::workerLoop, this);
    }

    // called at logout / exit: asks the thread to stop and waits for it
    void stop() {
        running.store(false);
        if (worker.joinable()) worker.join();
    }

    void setOpenPartner(int friendId) { openPartner.store(friendId); }
    int latestMessageId() const { return latest.load(); }

    int unreadFrom(int friendId) const {
        std::lock_guard<std::mutex> guard(dataMutex);
        std::map<int, int>::const_iterator it = unread.find(friendId);
        return it == unread.end() ? 0 : it->second;
    }

    int totalUnread() const {
        std::lock_guard<std::mutex> guard(dataMutex);
        int total = 0;
        for (std::map<int, int>::const_iterator it = unread.begin(); it != unread.end(); ++it) total += it->second;
        return total;
    }

    void clearUnread(int friendId) {
        std::lock_guard<std::mutex> guard(dataMutex);
        unread.erase(friendId);
    }

    void clearAllUnread() {
        std::lock_guard<std::mutex> guard(dataMutex);
        unread.clear();
    }
};

#endif //STEAM_CHAT_H
