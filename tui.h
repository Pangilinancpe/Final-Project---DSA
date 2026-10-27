#ifndef STEAM_TUI_H
#define STEAM_TUI_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>
  #include <io.h>
#else
  #include <unistd.h>
#endif

//==========================================
// TUI MODULE (text user interface)
// Colors (ANSI escape codes), aligned tables, banners and boxes.
// No extra library is needed, everything is plain text.
//
// Colors are only turned on when the program runs in a real terminal.
// They stay OFF when the output is redirected to a file, when the user starts the
// program with --plain, or when the NO_COLOR environment variable is set.
// Only ASCII characters are used for the borders, so every console shows them correctly.
//==========================================

bool tuiColor = false;

const char* const C_RESET   = "\033[0m";
const char* const C_BOLD    = "\033[1m";
const char* const C_DIM     = "\033[2m";
const char* const C_RED     = "\033[31m";
const char* const C_GREEN   = "\033[32m";
const char* const C_YELLOW  = "\033[33m";
const char* const C_BLUE    = "\033[34m";
const char* const C_MAGENTA = "\033[35m";
const char* const C_CYAN    = "\033[36m";
const char* const C_TITLE   = "\033[1;36m";    // bold cyan

const size_t TUI_WIDTH = 79;

// call once at the start. force = true keeps the colors even when the output is not a terminal
void tuiInit(bool allowColor, bool force = false) {
    tuiColor = false;
    if (!allowColor) return;
    if (std::getenv("NO_COLOR") != nullptr) return;
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out == INVALID_HANDLE_VALUE || !GetConsoleMode(out, &mode)) { tuiColor = force; return; }
    // Windows 10+ needs "virtual terminal processing" to understand ANSI colors
    tuiColor = SetConsoleMode(out, mode | 0x0004) != 0 || force;
#else
    tuiColor = force || (isatty(STDOUT_FILENO) != 0);
#endif
}

// colored text (plain text when colors are off)
std::string paint(const std::string& text, const char* color) {
    if (!tuiColor) return text;
    return std::string(color) + text + C_RESET;
}

//==========================
// layout helpers. ALWAYS pad first and paint after, because the color codes have no width
//==========================
std::string repeatChar(char c, size_t n) {
    return std::string(n, c);
}

// cut long text with "..." and pad short text with spaces to exactly 'width' characters
std::string fit(const std::string& s, size_t width) {
    std::string out = s;
    if (out.size() > width) {
        if (width > 3) out = out.substr(0, width - 3) + "...";
        else out = out.substr(0, width);
    }
    out.append(width - out.size(), ' ');
    return out;
}

std::string fitRight(const std::string& s, size_t width) {
    std::string out = s;
    if (out.size() > width) out = out.substr(0, width);
    return std::string(width - out.size(), ' ') + out;
}

std::string centerText(const std::string& s, size_t width) {
    if (s.size() >= width) return s.substr(0, width);
    size_t left = (width - s.size()) / 2;
    return std::string(left, ' ') + s + std::string(width - s.size() - left, ' ');
}

//==========================
// banner and section headers
//==========================
void tuiBanner(const std::string& title) {
    std::string line = "+" + repeatChar('-', TUI_WIDTH - 2) + "+";
    std::cout << "\n" << paint(line, C_TITLE) << "\n"
              << paint("|" + centerText(title, TUI_WIDTH - 2) + "|", C_TITLE) << "\n"
              << paint(line, C_TITLE) << "\n";
}

void tuiSection(const std::string& title) {
    std::string text = "== " + title + " ";
    if (text.size() < TUI_WIDTH) text += repeatChar('=', TUI_WIDTH - text.size());
    std::cout << "\n" << paint(text, C_TITLE) << "\n";
}

void tuiRule() {
    std::cout << paint(repeatChar('-', TUI_WIDTH), C_DIM) << "\n";
}

// "[tag1] [tag2]" in color
std::string tagChips(const std::vector<std::string>& tags) {
    std::string out;
    for (size_t i = 0; i < tags.size(); i++) {
        if (i > 0) out += " ";
        out += paint("[" + tags[i] + "]", C_MAGENTA);
    }
    return out;
}

//==========================
// numbered menu, two columns
//==========================
void tuiMenu(const std::vector<std::string>& labels, const std::string& zeroLabel) {
    std::vector<std::string> cells;
    for (size_t i = 0; i < labels.size(); i++) {
        std::string num = fitRight(std::to_string(i + 1), 2);
        cells.push_back(num + ") " + labels[i]);
    }
    cells.push_back(" 0) " + zeroLabel);

    const size_t colWidth = 36;
    std::cout << "\n";
    for (size_t i = 0; i < cells.size(); i += 2) {
        // color only the "NN)" part of each cell
        std::string left = cells[i];
        std::string leftNum = left.substr(0, 3);
        std::cout << "  " << paint(leftNum, C_YELLOW) << fit(left.substr(3), colWidth);
        if (i + 1 < cells.size()) {
            std::string right = cells[i + 1];
            std::cout << paint(right.substr(0, 3), C_YELLOW) << right.substr(3);
        }
        std::cout << "\n";
    }
}

#endif //STEAM_TUI_H
