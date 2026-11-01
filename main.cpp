#include "store.h"
#include <streambuf>
#include <mutex>

// options:  --plain = no colors     --color = force colors
//==========================================================================
// CLEAR SCREEN
// Clears the terminal so the old menus don't stay on screen.
//
// Simple idea:
//   1. The user answers a menu ("Choice: ")  ->  clear the screen, then show the result.
//   2. The program asks for more info ("Username: ")  ->  don't clear, the user still needs it.
//==========================================================================

// clears the terminal
void wipeConsole(std::streambuf* dest) {
    dest->pubsync();
#ifdef _WIN32
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hStdOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        DWORD count;
        DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
        COORD homeCoords = {0, 0};
        FillConsoleOutputCharacterA(hStdOut, ' ', cellCount, homeCoords, &count);
        FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, cellCount, homeCoords, &count);
        SetConsoleCursorPosition(hStdOut, homeCoords);
    }
#endif
    const char codes[] = "\033[2J\033[3J\033[H";
    dest->sputn(codes, (std::streamsize)(sizeof(codes) - 1));
    dest->pubsync();
}

// prompts after which the user is looking at a NEW screen
bool isScreenChangingPrompt(const std::string& p) {
    const char* starts[] = {"Choice: ", "Game id or title: ", "Search (", "Amount to add", "Chat with (", "Sort by: "};
    for (size_t i = 0; i < sizeof(starts) / sizeof(starts[0]); i++) {
        if (p.compare(0, std::string(starts[i]).size(), starts[i]) == 0) return true;
    }
    return false;
}

// filter on std::cout
class ScreenBuf : public std::streambuf {
private:
    std::streambuf* dest;
    std::mutex lock;               // the chat thread prints too
    std::string line;              // the unfinished line printed so far (a prompt, while the program waits)
    std::string prompt;            // the last prompt that was flushed
    bool pendingClear = false;     // the user moved on, clear before the next real output
    std::string held;              // output that is waiting while pendingClear is true

    void releaseHeld() {
        if (!held.empty()) dest->sputn(held.data(), (std::streamsize)held.size());
        held.clear();
    }

    void write(const char* s, std::streamsize n) {
        for (std::streamsize i = 0; i < n; i++) {
            if (s[i] == '\n') line.clear();
            else line.push_back(s[i]);
        }
        if (!pendingClear) { dest->sputn(s, n); return; }

        held.append(s, (size_t)n);
        if (held.find('\n') != std::string::npos) {     // a result or a new screen is coming: clear first
            pendingClear = false;
            wipeConsole(dest);
            releaseHeld();
        }
    }
public:
    explicit ScreenBuf(std::streambuf* target) : dest(target) {}
    // called by LineBuf when the user pressed Enter
    void userPressedEnter() {
        std::lock_guard<std::mutex> guard(lock);
        bool changed = isScreenChangingPrompt(prompt);
        prompt.clear();
        line.clear();
        if (changed) pendingClear = true;
    }
    // clear right now (used once at the start)
    void clearNow() {
        std::lock_guard<std::mutex> guard(lock);
        wipeConsole(dest);
    }
    // give back anything that is still waiting
    void finish() {
        std::lock_guard<std::mutex> guard(lock);
        pendingClear = false;
        releaseHeld();
        dest->pubsync();
    }

protected:
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) return traits_type::not_eof(ch);
        char c = traits_type::to_char_type(ch);
        std::lock_guard<std::mutex> guard(lock);
        write(&c, 1);
        return ch;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override {
        std::lock_guard<std::mutex> guard(lock);
        write(s, n);
        return n;
    }

    int sync() override {
        std::lock_guard<std::mutex> guard(lock);
        // a flush with unfinished text and nothing else is a prompt (the program waits for the user).
        // the user still needs the old screen to answer it, so cancel the clear.
        if (pendingClear && !held.empty() && held.find('\n') == std::string::npos) {
            pendingClear = false;
            releaseHeld();
        }
        if (!line.empty()) prompt = line;
        return dest->pubsync();
    }
};

// filter on std::cin: hands over one line at a time and tells ScreenBuf when a line was entered
class LineBuf : public std::streambuf {
private:
    std::streambuf* src;
    ScreenBuf* screen;
    std::string buffer;

public:
    LineBuf(std::streambuf* source, ScreenBuf* out) : src(source), screen(out) {}

protected:
    int_type underflow() override {
        if (gptr() < egptr()) return traits_type::to_int_type(*gptr());

        buffer.clear();
        int_type c;
        while (!traits_type::eq_int_type(c = src->sbumpc(), traits_type::eof())) {
            buffer.push_back(traits_type::to_char_type(c));
            if (buffer.back() == '\n') break;
        }
        if (buffer.empty()) return traits_type::eof();

        screen->userPressedEnter();
        setg(&buffer[0], &buffer[0], &buffer[0] + buffer.size());
        return traits_type::to_int_type(*gptr());
    }
};

bool isRealTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

int main(int argc, char* argv[]) {
    bool allowColor = true;
    bool forceColor = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--plain") allowColor = false;
        if (arg == "--color") forceColor = true;
    }
    tuiInit(allowColor, forceColor);

    if (!isRealTerminal()) {
        runStore();                         // output to a file or piped input: no clearing
        return 0;
    }

    std::streambuf* originalOut = std::cout.rdbuf();
    std::streambuf* originalIn = std::cin.rdbuf();
    ScreenBuf screen(originalOut);
    LineBuf input(originalIn, &screen);
    std::cout.rdbuf(&screen);
    std::cin.rdbuf(&input);

    screen.clearNow();
    runStore();

    screen.finish();
    std::cout.rdbuf(originalOut);
    std::cin.rdbuf(originalIn);
    return 0;
}
