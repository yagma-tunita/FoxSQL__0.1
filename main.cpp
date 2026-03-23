#include "Database.h"
#include "help.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <windows.h>

void SetColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void ResetColor() {
    SetColor(7);
}

void HighlightPrint(const std::string& text) {
    const std::vector<std::string> keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES",
        "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "INT",
        "VARCHAR", "PRIMARY", "KEY", "AND", "OR", "NOT"
    };

    size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            std::cout << text[pos];
            ++pos;
        }
        if (pos >= text.size()) break;

        if (std::isalpha(static_cast<unsigned char>(text[pos])) || text[pos] == '_') {
            size_t start = pos;
            while (pos < text.size() && (std::isalnum(static_cast<unsigned char>(text[pos])) || text[pos] == '_')) ++pos;
            std::string word = text.substr(start, pos - start);
            std::string upperWord = word;
            std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);

            if (std::find(keywords.begin(), keywords.end(), upperWord) != keywords.end()) {
                SetColor(9);
                std::cout << word;
                ResetColor();
            }
            else {
                std::cout << word;
            }
        }
        else if (text[pos] == '\'') {
            size_t start = pos;
            ++pos;
            while (pos < text.size() && text[pos] != '\'') ++pos;
            ++pos;
            std::string str = text.substr(start, pos - start);
            SetColor(12);
            std::cout << str;
            ResetColor();
        }
        else if (std::isdigit(static_cast<unsigned char>(text[pos])) ||
            (text[pos] == '-' && pos + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[pos + 1])))) {
            size_t start = pos;
            if (text[pos] == '-') ++pos;
            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) ++pos;
            std::string num = text.substr(start, pos - start);
            SetColor(10);
            std::cout << num;
            ResetColor();
        }
        else {
            std::cout << text[pos];
            ++pos;
        }
    }
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static std::string toLower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

int main() {
    SetConsoleTitle(L"FoxSQL 0.1 - SQL Terminal");

    FoxSQL::Database db;

    SetColor(11);
    std::cout << "\n========================================\n";
    std::cout << "    Welcome to FoxSQL 0.1\n";
    std::cout << "    Lightweight SQL Database Engine\n";
    std::cout << "========================================\n";
    ResetColor();
    std::cout << "Type 'help' for commands, 'quit' to exit.\n";
    std::cout << "SQL statements can be terminated with a semicolon (optional).\n\n";

    std::string line;
    while (true) {
        std::cout << "foxsql> ";
        std::getline(std::cin, line);
        line = trim(line);
        if (line.empty()) continue;

        std::string cmd = toLower(line);
        if (cmd == "quit" || cmd == "exit") {
            std::cout << "Goodbye!\n";
            break;
        }
        else if (cmd == "help") {
            FoxSQL::Help::show();
            continue;
        }

        try {
            auto result = db.executeSQL(line);
            if (!result.empty()) {
                std::cout << "id\tname\n";
                std::cout << "---\t----\n";
                for (const auto& row : result) {
                    try {
                        int64_t id = row.getValue("id").getInt();
                        std::string name = row.getValue("name").getString();
                        std::cout << id << "\t" << name << std::endl;
                    }
                    catch (...) {}
                }
            }
            else {
                std::cout << "Query executed successfully.\n";
            }
        }
        catch (const std::exception& e) {
            SetColor(12);
            std::cerr << "Error: " << e.what() << std::endl;
            ResetColor();
        }
    }

    db.shutdown();
    return 0;
}