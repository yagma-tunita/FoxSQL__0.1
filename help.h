// help.h
#pragma once
#include <iostream>
#include <string>

namespace FoxSQL {

    class Help {
    public:
        static void show() {
            std::cout << "\n=== FoxSQL Help ===\n";
            std::cout << "Supported SQL commands:\n";
            std::cout << "  CREATE TABLE <name> (col1 TYPE [PRIMARY KEY], col2 TYPE, ...)\n";
            std::cout << "    Example: CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100))\n";
            std::cout << "  INSERT INTO table_name VALUES (val1, val2, ...)\n";
            std::cout << "    Example: INSERT INTO users VALUES (1, 'Alice')\n";
            std::cout << "  SELECT * FROM table_name [WHERE condition]\n";
            std::cout << "    Example: SELECT * FROM users WHERE id = 2\n";
            std::cout << "  UPDATE table_name SET col = val [WHERE condition]\n";
            std::cout << "  DELETE FROM table_name [WHERE condition]\n";
            std::cout << "\nComparison operators: =, >, <, >=, <=, <>, !=\n";
            std::cout << "Logical operators: AND, OR\n\n";
            std::cout << "Special commands:\n";
            std::cout << "  help     - show this help\n";
            std::cout << "  quit     - discard all changes and exit\n";
            std::cout << "  exit     - save all changes and exit\n";
            std::cout << "========================\n";
        }
    };

} 