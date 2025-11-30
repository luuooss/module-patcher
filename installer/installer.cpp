#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <conio.h>
#include <thread>
#include <chrono>
#include <random>
#include <fstream>
#include "startup_sound.h"

namespace fs = std::filesystem;

void mettrecouleurdemerde(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void cpasvrmaucentermgl(const std::string& text, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int x = (width - (int)text.length()) / 2;
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hConsole, pos);
    std::cout << text;
}

void initcls() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(hInput, &mode)) {
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        mode &= ~ENABLE_MOUSE_INPUT;
        SetConsoleMode(hInput, mode);
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    SHORT width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    SHORT height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    COORD newSize = { width, height };
    SetConsoleScreenBufferSize(hConsole, newSize);
    HWND hwnd = GetConsoleWindow();
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
    SetWindowLong(hwnd, GWL_STYLE, style);
}

void whenopen() {
    std::vector<std::string> lines = {
        "========================================",
        "         Discord Stereo Patcher         ",
        "========================================",
        "",
        "Loading..."
    };

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    for (size_t i = 0; i < lines.size(); i++) {
        int x = (width - (int)lines[i].length()) / 2;
        COORD pos = { (SHORT)x, (SHORT)i + 2 };
        SetConsoleCursorPosition(hConsole, pos);
        for (char c : lines[i]) {
            std::cout << c;
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        std::cout << "\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    system("cls");
}

std::string RandomString(size_t length) {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);
    std::string result;
    for (size_t i = 0; i < length; ++i) result += chars[dist(rng)];
    return result;
}

void clstitlecustom() {
    while (true) {
        std::string title = RandomString(32) + " github: 2ufdev";
        SetConsoleTitleA(title.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::string GetLocalAppData() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) return std::string(path);
    return "";
}

std::vector<fs::path> GetDiscordInstallations() {
    std::vector<fs::path> paths;
    std::string localAppData = GetLocalAppData();
    std::vector<std::string> variants = { "Discord", "DiscordPTB", "DiscordCanary" };
    for (auto& var : variants) {
        fs::path discordPath = fs::path(localAppData) / var;
        if (fs::exists(discordPath)) paths.push_back(discordPath);
    }
    return paths;
}

fs::path GetLatestVersion(const fs::path& basePath) {
    fs::path latest;
    for (auto& entry : fs::directory_iterator(basePath)) {
        if (entry.is_directory() && entry.path().filename().string().rfind("app-1.0.", 0) == 0)
            latest = entry.path();
    }
    return latest;
}

void CloseDiscordProcesses() {
    std::vector<std::string> processes = { "Discord.exe", "DiscordPTB.exe", "DiscordCanary.exe" };
    for (auto& proc : processes) {
        std::string cmd = "taskkill /F /IM " + proc + " >nul 2>&1";
        system(cmd.c_str());
    }
}

void copydirectory(const fs::path& source, const fs::path& destination) {
    std::vector<fs::path> files;
    for (auto& entry : fs::recursive_directory_iterator(source)) {
        if (entry.is_regular_file()) files.push_back(entry.path());
        else if (entry.is_directory()) fs::create_directories(destination / fs::relative(entry.path(), source));
    }

    size_t total = files.size();
    for (size_t i = 0; i < total; i++) {
        fs::path targetPath = destination / fs::relative(files[i], source);
        fs::copy_file(files[i], targetPath, fs::copy_options::overwrite_existing);
        int percent = (int)((i + 1) * 100 / total);
        std::cout << "\rCopying files... " << percent << "%";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    std::cout << "\n";
}

void PatchDiscord(const fs::path& discordPath, const fs::path& replacementPath) {
    fs::path latestVersion = GetLatestVersion(discordPath);
    if (latestVersion.empty()) {
        mettrecouleurdemerde(12);
        std::cout << "No app version found for " << discordPath.filename().string() << "\n";
        mettrecouleurdemerde(7);
        return;
    }

    fs::path modulesPath = latestVersion / "modules";
    for (auto& moduleEntry : fs::directory_iterator(modulesPath)) {
        if (moduleEntry.is_directory() && moduleEntry.path().filename().string().find("discord_voice-") == 0) {
            fs::path targetPath = moduleEntry.path() / "discord_voice";
            if (fs::exists(targetPath)) {
                std::cout << "Copying files to " << discordPath.filename().string() << "...\n";
                copydirectory(replacementPath, targetPath);
                mettrecouleurdemerde(10);
                std::cout << "Files copied successfully!\n";
                mettrecouleurdemerde(7);
            }
        }
    }
}

int menuuhq(const std::vector<std::string>& options, int startY) {
    int selected = 0;
    while (true) {
        system("cls");
        cpasvrmaucentermgl("========================================", 0);
        cpasvrmaucentermgl("         Discord Stereo Patcher         ", 1);
        cpasvrmaucentermgl("========================================", 2);
        cpasvrmaucentermgl("Use arrow keys to select, Enter to confirm.", startY - 1);

        for (size_t i = 0; i < options.size(); i++) {
            std::string line = (i == selected ? "> " : "  ") + options[i];
            mettrecouleurdemerde(i == selected ? 11 : 7);
            cpasvrmaucentermgl(line, startY + (int)i);
        }
        mettrecouleurdemerde(7);

        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == 72 && selected > 0) selected--;
            if (key == 80 && selected < (int)options.size() - 1) selected++;
        }
        else if (key == 13) return selected;
    }
}

void hidecaret() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void startupsound() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string tempFile = std::string(tempPath) + "discord_launch_sound.mp3";
    std::ofstream f(tempFile, std::ios::binary);
    if (!f) {
        std::cerr << "err temp file %TEMP%\n";
        return;
    }
    f.write(reinterpret_cast<const char*>(launch_mp3), launch_mp3_len);
    f.close();

    std::string openCmd = "open \"" + tempFile + "\" type mpegvideo alias startupSound";
    mciSendStringA(openCmd.c_str(), NULL, 0, NULL);
    mciSendStringA("setaudio startupSound volume to 300", NULL, 0, NULL);
    mciSendStringA("play startupSound", NULL, 0, NULL);
    std::thread([tempFile]() {
        Sleep(5000);
        DeleteFileA(tempFile.c_str());
        }).detach();
}


int main() {

    startupsound();
    initcls();
    //CloseDiscordProcesses();
    std::thread titleThread(clstitlecustom);
    titleThread.detach();
    whenopen();
    hidecaret();
    fs::path exePath = fs::current_path();
    fs::path replacementPath = exePath / "discord_voice";

    if (!fs::exists(replacementPath)) {
        mettrecouleurdemerde(12);
        std::cerr << "'discord_voice' folder not found next to the executable.\n";
        mettrecouleurdemerde(7);
        system("pause");
        return 1;
    }

    while (true) {
        auto discordPaths = GetDiscordInstallations();
        if (discordPaths.empty()) {
            mettrecouleurdemerde(12);
            std::cout << "No Discord installations found.\n";
            mettrecouleurdemerde(7);
            system("pause");
            return 1;
        }

        std::vector<std::string> menuOptions = { "Patch ALL Discord" };
        for (auto& path : discordPaths) menuOptions.push_back(path.filename().string());
        menuOptions.push_back("Exit");

        int choice = menuuhq(menuOptions, 5);
        if (choice == 0) {
            for (auto& path : discordPaths) PatchDiscord(path, replacementPath);
            mettrecouleurdemerde(10);
            cpasvrmaucentermgl("All Discord installations patched!", 15);
            mettrecouleurdemerde(7);
            system("pause");
        }
        else if ((size_t)choice == menuOptions.size() - 1) break;
        else {
            PatchDiscord(discordPaths[choice - 1], replacementPath);
            system("pause");
        }
    }
    return 0;
}
