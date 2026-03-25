#pragma once
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <windows.h>

namespace FoxSQL {
    namespace Color {
        // 颜色常量
        constexpr int DEFAULT = 7;    // 白色
        constexpr int KEYWORD = 9;    // 蓝色
        constexpr int STRING = 12;    // 红色
        constexpr int NUMBER = 10;    // 绿色
        constexpr int CYE = 11;       //青色

        // 设置控制台颜色
        inline void set(int color) {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, color);
        }

        // 恢复默认颜色
        inline void reset() {
            set(DEFAULT);
        }

        // 高亮打印 SQL 语句（关键字、字符串、数字分别着色）
        inline void highlightPrint(const std::string& text) {
            const std::vector<std::string> keywords = {
                "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES",
                "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "INT",
                "VARCHAR", "PRIMARY", "KEY", "AND", "OR", "NOT"
            };

            size_t pos = 0;
            while (pos < text.size()) {
                // 跳过空白
                while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
                    std::cout << text[pos];
                    ++pos;
                }
                if (pos >= text.size()) break;

                // 单词（标识符或关键字）
                if (std::isalpha(static_cast<unsigned char>(text[pos])) || text[pos] == '_') {
                    size_t start = pos;
                    while (pos < text.size() && (std::isalnum(static_cast<unsigned char>(text[pos])) || text[pos] == '_')) ++pos;
                    std::string word = text.substr(start, pos - start);
                    std::string upperWord = word;
                    std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);

                    if (std::find(keywords.begin(), keywords.end(), upperWord) != keywords.end()) {
                        set(KEYWORD);
                        std::cout << word;
                        reset();
                    }
                    else {
                        std::cout << word;
                    }
                }
                // 字符串字面量
                else if (text[pos] == '\'') {
                    size_t start = pos;
                    ++pos;
                    while (pos < text.size() && text[pos] != '\'') ++pos;
                    ++pos;
                    std::string str = text.substr(start, pos - start);
                    set(STRING);
                    std::cout << str;
                    reset();
                }
                // 数字（包括负数）
                else if (std::isdigit(static_cast<unsigned char>(text[pos])) ||
                    (text[pos] == '-' && pos + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[pos + 1])))) {
                    size_t start = pos;
                    if (text[pos] == '-') ++pos;
                    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) ++pos;
                    std::string num = text.substr(start, pos - start);
                    set(NUMBER);
                    std::cout << num;
                    reset();
                }
                else {
                    std::cout << text[pos];
                    ++pos;
                }
            }
        }
    }
}