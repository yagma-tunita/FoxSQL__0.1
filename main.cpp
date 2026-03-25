#include "Database.h"
#include "help.h"
#include "Color.h"      // 新增颜色头文件
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <windows.h>

void SetColor(int color) {
    FoxSQL::Color::set(color);
}

void ResetColor() {
    FoxSQL::Color::reset();
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

    // 使用蓝色显示欢迎信息
    SetColor(FoxSQL::Color::KEYWORD);
    std::cout << "\n========================================\n";
    std::cout << "    Welcome to FoxSQL 0.1\n";
    std::cout << "    Lightweight SQL Database Engine\n";
    std::cout << "========================================\n";
    ResetColor();
    std::cout << "Type 'help' for commands.\n";
    std::cout << "  'quit' : discard all changes and exit\n";
    std::cout << "  'exit' : save all changes and exit\n";
    std::cout << "SQL statements can be terminated with a semicolon (optional).\n\n";

    std::string line;
    while (true) {
        std::cout << "foxsql> ";
        std::getline(std::cin, line);
        line = trim(line);
        if (line.empty()) continue;

        std::string cmd = toLower(line);
        if (cmd == "quit") {
            std::cout << "Exiting without saving changes...\n";
            break;
        }
        else if (cmd == "exit") {
            std::cout << "Saving changes and exiting...\n";
            db.save();
            break;
        }
        else if (cmd == "help") {
            FoxSQL::Help::show();
            continue;
        }

        try {
            auto result = db.executeSQL(line);
            if (!result.empty()) {
                // 使用颜色显示表头
                SetColor(FoxSQL::Color::KEYWORD);
                std::cout << "id\tname\n";
                SetColor(FoxSQL::Color::DEFAULT);
                std::cout << "---\t----\n";

                for (const auto& row : result) {
                    try {
                        int64_t id = row.getValue("id").getInt();
                        std::string name = row.getValue("name").getString();
                        // id 显示为绿色
                        SetColor(FoxSQL::Color::NUMBER);
                        std::cout << id;
                        ResetColor();
                        std::cout << "\t";
                        // name 显示为青色
                        SetColor(FoxSQL::Color::CYE);
                        std::cout << name;
                        ResetColor();
                        std::cout << std::endl;
                    }
                    catch (...) {}
                }
            }
            else {
                std::cout << "Query executed successfully.\n";
            }
        }
        catch (const std::exception& e) {
            SetColor(FoxSQL::Color::STRING);   // 错误信息红色
            std::cerr << "Error: " << e.what() << std::endl;
            ResetColor();
        }
    }

    return 0;
}
