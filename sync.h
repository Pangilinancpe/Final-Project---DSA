#ifndef STEAM_SYNC_H
#define STEAM_SYNC_H

#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <system_error>

//==========================================
// SYNC MODULE (shared helpers for threads and multiple terminals)
//
// Two kinds of "sharing" exist in this project:
//   1. threads inside ONE program   -> std::mutex (consoleMutex below)
//   2. several programs (terminals) -> FileLock + atomic file replace
//==========================================

//--------------------------
// every thread that prints while another thread may print too
// should hold this mutex, so lines do not get mixed up
//--------------------------
std::mutex& consoleMutex() {
    static std::mutex m;
    return m;
}

//--------------------------
// FileLock: a lock shared between different programs.
// It works by creating a folder called "<name>.lock":
// creating a folder is atomic, so only ONE program can succeed.
// The lock is released automatically when the object goes out of scope.
// A lock older than 10 seconds is treated as left behind by a crashed program.
//--------------------------
class FileLock {
private:
    std::string path;
    bool held = false;

public:
    explicit FileLock(const std::string& name, int timeoutMs = 3000) {
        namespace fs = std::filesystem;
        path = name + ".lock";
        auto start = std::chrono::steady_clock::now();

        while (true) {
            std::error_code ec;
            if (fs::create_directory(path, ec) && !ec) {      // we got it
                held = true;
                return;
            }

            // somebody holds it: is that lock stale (crashed program)?
            ec.clear();
            auto stamp = fs::last_write_time(path, ec);
            if (!ec) {
                auto age = fs::file_time_type::clock::now() - stamp;
                if (age > std::chrono::seconds(10)) {
                    fs::remove(path, ec);
                    continue;
                }
            }

            auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start).count();
            if (waited > timeoutMs) return;                    // give up, held stays false
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    ~FileLock() {
        if (held) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    }

    bool locked() const { return held; }

    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
};

//--------------------------
// replace a file in one step, so another program never reads a half written file.
// (write to "<file>.tmp" first, then rename it over the real file)
//--------------------------
bool replaceFile(const std::string& tmp, const std::string& finalName) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::rename(tmp, finalName, ec);
    if (!ec) return true;

    // some systems refuse to rename over an existing file: remove it, then retry
    ec.clear();
    fs::remove(finalName, ec);
    ec.clear();
    fs::rename(tmp, finalName, ec);
    return !ec;
}

#endif //STEAM_SYNC_H
