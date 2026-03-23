#pragma once
#include "AST.h"
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <memory>
#include <sstream>

namespace FoxSQL 
{

    class SQLParser {
    public:
        static std::unique_ptr<ASTNode> parse(const std::string& sql) {
            Parser p(sql);
            return p.parseStatement();
        }

    private:
        struct Token {
            enum Type {
                TOK_EOF,
                TOK_CREATE, TOK_TABLE, TOK_INSERT, TOK_INTO, TOK_VALUES,
                TOK_SELECT, TOK_FROM, TOK_WHERE, TOK_DELETE, TOK_UPDATE,
                TOK_SET, TOK_AND, TOK_OR, TOK_PRIMARY, TOK_KEY,
                TOK_IDENTIFIER, TOK_INTEGER, TOK_STRING,
                TOK_LPAREN, TOK_RPAREN, TOK_COMMA, TOK_STAR, TOK_SEMICOLON,
                TOK_EQ, TOK_NE, TOK_LT, TOK_LE, TOK_GT, TOK_GE
            };
            Type type;
            std::string value;
            size_t pos;
        };

        class Parser {
        public:
            Parser(const std::string& src) : src_(src), pos_(0) {
                nextToken();
            }

            std::unique_ptr<ASTNode> parseStatement() {
                if (match(Token::TOK_CREATE)) {
                    expect(Token::TOK_TABLE);
                    return parseCreateTable();
                }
                else if (match(Token::TOK_INSERT)) {
                    expect(Token::TOK_INTO);
                    return parseInsert();
                }
                else if (match(Token::TOK_SELECT)) {
                    return parseSelect();
                }
                else if (match(Token::TOK_DELETE)) {
                    expect(Token::TOK_FROM);
                    return parseDelete();
                }
                else if (match(Token::TOK_UPDATE)) {
                    return parseUpdate();
                }
                else {
                    error("Unknown statement");
                }
                return nullptr;  
            }

        private:
            std::string src_;
            size_t pos_;
            Token current_;

            void nextToken() {
                skipWhitespace();
                if (pos_ >= src_.size()) {
                    current_.type = Token::TOK_EOF;
                    return;
                }
                current_.pos = pos_;
                char c = src_[pos_];
                if (c == '(') { current_.type = Token::TOK_LPAREN; pos_++; return; }
                if (c == ')') { current_.type = Token::TOK_RPAREN; pos_++; return; }
                if (c == ',') { current_.type = Token::TOK_COMMA; pos_++; return; }
                if (c == '*') { current_.type = Token::TOK_STAR; pos_++; return; }
                if (c == ';') { current_.type = Token::TOK_SEMICOLON; pos_++; return; }
                if (c == '=') { current_.type = Token::TOK_EQ; pos_++; return; }
                if (c == '<') {
                    pos_++;
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        current_.type = Token::TOK_LE;
                        pos_++;
                    }
                    else if (pos_ < src_.size() && src_[pos_] == '>') {
                        current_.type = Token::TOK_NE;
                        pos_++;
                    }
                    else {
                        current_.type = Token::TOK_LT;
                    }
                    return;
                }
                if (c == '>') {
                    pos_++;
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        current_.type = Token::TOK_GE;
                        pos_++;
                    }
                    else {
                        current_.type = Token::TOK_GT;
                    }
                    return;
                }
                if (c == '!') {
                    pos_++;
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        current_.type = Token::TOK_NE;
                        pos_++;
                    }
                    else {
                        error("Unexpected '!'");
                    }
                    return;
                }
                if (c == '\'') {
                    parseString();
                    return;
                }
                if (std::isdigit(c) || (c == '-' && pos_ + 1 < src_.size() && std::isdigit(src_[pos_ + 1]))) {
                    parseNumber();
                    return;
                }
                if (std::isalpha(c) || c == '_') {
                    parseKeywordOrIdentifier();
                    return;
                }
                error("Unexpected character");
            }

            void skipWhitespace() {
                while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_])))
                    pos_++;
            }

            void parseString() {
                pos_++; // skip '
                size_t start = pos_;
                while (pos_ < src_.size() && src_[pos_] != '\'') {
                    if (src_[pos_] == '\\') pos_++; // escape
                    pos_++;
                }
                if (pos_ >= src_.size()) error("Unterminated string");
                current_.value = src_.substr(start, pos_ - start);
                pos_++; // skip '
                current_.type = Token::TOK_STRING;
            }

            void parseNumber() {
                size_t start = pos_;
                if (src_[pos_] == '-') pos_++;
                while (pos_ < src_.size() && std::isdigit(src_[pos_])) pos_++;
                current_.value = src_.substr(start, pos_ - start);
                current_.type = Token::TOK_INTEGER;
            }

            void parseKeywordOrIdentifier() {
                size_t start = pos_;
                while (pos_ < src_.size() && (std::isalnum(src_[pos_]) || src_[pos_] == '_')) pos_++;
                current_.value = src_.substr(start, pos_ - start);
                std::string upper = toUpper(current_.value);
                if (upper == "CREATE")      current_.type = Token::TOK_CREATE;
                else if (upper == "TABLE")  current_.type = Token::TOK_TABLE;
                else if (upper == "INSERT") current_.type = Token::TOK_INSERT;
                else if (upper == "INTO")   current_.type = Token::TOK_INTO;
                else if (upper == "VALUES") current_.type = Token::TOK_VALUES;
                else if (upper == "SELECT") current_.type = Token::TOK_SELECT;
                else if (upper == "FROM")   current_.type = Token::TOK_FROM;
                else if (upper == "WHERE")  current_.type = Token::TOK_WHERE;
                else if (upper == "DELETE") current_.type = Token::TOK_DELETE;
                else if (upper == "UPDATE") current_.type = Token::TOK_UPDATE;
                else if (upper == "SET")    current_.type = Token::TOK_SET;
                else if (upper == "AND")    current_.type = Token::TOK_AND;
                else if (upper == "OR")     current_.type = Token::TOK_OR;
                else if (upper == "PRIMARY")current_.type = Token::TOK_PRIMARY;
                else if (upper == "KEY")    current_.type = Token::TOK_KEY;
                else current_.type = Token::TOK_IDENTIFIER;
            }

            std::string toUpper(const std::string& s) {
                std::string res = s;
                for (char& c : res) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                return res;
            }

            bool match(Token::Type type) {
                if (current_.type == type) {
                    nextToken();
                    return true;
                }
                return false;
            }

            void expect(Token::Type type) {
                if (current_.type != type) error("Expected token");
                nextToken();
            }

            void error(const std::string& msg) {
                throw SQLException("SQL parse error at " + std::to_string(current_.pos) + ": " + msg);
            }

            std::unique_ptr<CreateTableStmt> parseCreateTable() {
                auto stmt = std::make_unique<CreateTableStmt>();
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected table name");
                stmt->tableName = current_.value;
                nextToken();

                expect(Token::TOK_LPAREN);
                while (true) {
                    if (current_.type == Token::TOK_RPAREN) break;
                    parseColumnDefinition(*stmt);
                    if (current_.type == Token::TOK_COMMA) {
                        nextToken();
                        continue;
                    }
                    else if (current_.type == Token::TOK_RPAREN) {
                        break;
                    }
                    else {
                        error("Expected ',' or ')'");
                    }
                }
                expect(Token::TOK_RPAREN);
                return stmt;
            }

            void parseColumnDefinition(CreateTableStmt& stmt) {
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected column name");
                std::string colName = current_.value;
                nextToken();
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected column type");
                std::string typeStr = current_.value;
                nextToken();

                ColumnType type = (toUpper(typeStr) == "INT") ? ColumnType::INT : ColumnType::VARCHAR;
                size_t maxLen = 0;
                if (type == ColumnType::VARCHAR) {
                    if (current_.type == Token::TOK_LPAREN) {
                        nextToken();
                        if (current_.type != Token::TOK_INTEGER) error("Expected length");
                        maxLen = std::stoul(current_.value);
                        nextToken();
                        expect(Token::TOK_RPAREN);
                    }
                }

                bool isPrimary = false;
                if (current_.type == Token::TOK_PRIMARY) {
                    nextToken();
                    if (current_.type != Token::TOK_KEY) error("Expected KEY after PRIMARY");
                    nextToken();
                    isPrimary = true;
                }
                stmt.columns.emplace_back(colName, type, maxLen, isPrimary);
            }

            std::unique_ptr<InsertStmt> parseInsert() {
                auto stmt = std::make_unique<InsertStmt>();
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected table name");
                stmt->tableName = current_.value;
                nextToken();
                expect(Token::TOK_VALUES);
                expect(Token::TOK_LPAREN);
                while (true) {
                    Value val = parseValue();
                    stmt->values.emplace_back("", val); 
                    if (current_.type == Token::TOK_COMMA) {
                        nextToken();
                        continue;
                    }
                    else if (current_.type == Token::TOK_RPAREN) {
                        break;
                    }
                    else {
                        error("Expected ',' or ')'");
                    }
                }
                expect(Token::TOK_RPAREN);
                return stmt;
            }

            Value parseValue() {
                if (current_.type == Token::TOK_INTEGER) {
                    int64_t v = std::stoll(current_.value);
                    nextToken();
                    return Value(v);
                }
                else if (current_.type == Token::TOK_STRING) {
                    std::string s = current_.value;
                    nextToken();
                    return Value(s);
                }
                else {
                    error("Expected value");
                }
                return Value(0); 
            }

            std::unique_ptr<SelectStmt> parseSelect() {
                auto stmt = std::make_unique<SelectStmt>();
                if (current_.type == Token::TOK_STAR) {
                    stmt->columns.clear(); 
                    nextToken();
                }
                else {
                    while (true) {
                        if (current_.type != Token::TOK_IDENTIFIER) error("Expected column name");
                        stmt->columns.push_back(current_.value);
                        nextToken();
                        if (current_.type == Token::TOK_COMMA) {
                            nextToken();
                            continue;
                        }
                        else {
                            break;
                        }
                    }
                }
                expect(Token::TOK_FROM);
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected table name");
                stmt->tableName = current_.value;
                nextToken();

                if (current_.type == Token::TOK_WHERE) {
                    nextToken();
                    stmt->where = parseExpression();
                }
                return stmt;
            }

            std::unique_ptr<DeleteStmt> parseDelete() {
                auto stmt = std::make_unique<DeleteStmt>();
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected table name");
                stmt->tableName = current_.value;
                nextToken();

                if (current_.type == Token::TOK_WHERE) {
                    nextToken();
                    stmt->where = parseExpression();
                }
                return stmt;
            }

            std::unique_ptr<UpdateStmt> parseUpdate() {
                auto stmt = std::make_unique<UpdateStmt>();
                if (current_.type != Token::TOK_IDENTIFIER) error("Expected table name");
                stmt->tableName = current_.value;
                nextToken();
                expect(Token::TOK_SET);

                while (true) {
                    if (current_.type != Token::TOK_IDENTIFIER) error("Expected column name");
                    std::string col = current_.value;
                    nextToken();
                    expect(Token::TOK_EQ);
                    Value val = parseValue();
                    stmt->setClause.emplace_back(col, val);

                    if (current_.type == Token::TOK_COMMA) {
                        nextToken();
                        continue;
                    }
                    else {
                        break;
                    }
                }

                if (current_.type == Token::TOK_WHERE) {
                    nextToken();
                    stmt->where = parseExpression();
                }
                return stmt;
            }

            std::unique_ptr<Expression> parseExpression() {
                auto left = parseComparison();
                while (current_.type == Token::TOK_AND || current_.type == Token::TOK_OR) {
                    auto op = current_.type;
                    nextToken();
                    auto right = parseComparison();
                    auto expr = std::make_unique<Expression>();
                    expr->type = (op == Token::TOK_AND) ? Expression::AND : Expression::OR;
                    expr->left = std::move(left);
                    expr->right = std::move(right);
                    left = std::move(expr);
                }
                return left;
            }

            std::unique_ptr<Expression> parseComparison() {
                auto left = parsePrimary();
                if (isComparisonOp(current_.type)) {
                    auto op = current_.type;
                    nextToken();
                    auto right = parsePrimary();
                    auto expr = std::make_unique<Expression>();
                    expr->type = tokenToExprType(op);
                    expr->left = std::move(left);
                    expr->right = std::move(right);
                    return expr;
                }
                return left;
            }

            std::unique_ptr<Expression> parsePrimary() {
                if (current_.type == Token::TOK_IDENTIFIER) {
                    auto expr = std::make_unique<Expression>();
                    expr->type = Expression::COLUMN;
                    expr->column = current_.value;
                    nextToken();
                    return expr;
                }
                else if (current_.type == Token::TOK_INTEGER) {
                    auto expr = std::make_unique<Expression>();
                    expr->type = Expression::LITERAL;
                    expr->literal = Value(std::stoll(current_.value));
                    nextToken();
                    return expr;
                }
                else if (current_.type == Token::TOK_STRING) {
                    auto expr = std::make_unique<Expression>();
                    expr->type = Expression::LITERAL;
                    expr->literal = Value(current_.value);
                    nextToken();
                    return expr;
                }
                else if (current_.type == Token::TOK_LPAREN) {
                    nextToken();
                    auto expr = parseExpression();
                    expect(Token::TOK_RPAREN);
                    return expr;
                }
                else {
                    error("Unexpected token in expression");
                }
                return nullptr; 
            }

            bool isComparisonOp(Token::Type t) {
                return t == Token::TOK_EQ || t == Token::TOK_NE || t == Token::TOK_LT ||
                    t == Token::TOK_LE || t == Token::TOK_GT || t == Token::TOK_GE;
            }

            Expression::Type tokenToExprType(Token::Type t) {
                switch (t) {
                case Token::TOK_EQ: return Expression::EQ;
                case Token::TOK_NE: return Expression::NE;
                case Token::TOK_LT: return Expression::LT;
                case Token::TOK_LE: return Expression::LE;
                case Token::TOK_GT: return Expression::GT;
                case Token::TOK_GE: return Expression::GE;
                default: throw SQLException("Invalid comparison operator");
                }
            }
        };
    };

}