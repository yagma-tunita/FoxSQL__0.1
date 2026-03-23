#pragma once
#include "common.h"
#include "Row.h"
#include <string>
#include <vector>
#include <memory>

namespace FoxSQL 
{

    struct ASTNode 
    {
        virtual ~ASTNode() = default;
    };

    struct CreateTableStmt : ASTNode {
        std::string tableName;
        std::vector<ColumnMeta> columns;
    };

    struct InsertStmt : ASTNode 
    {
        std::string tableName;
        std::vector<std::pair<std::string, Value>> values; 
    };

    struct SelectStmt : ASTNode 
    {
        std::string tableName;
        std::vector<std::string> columns;   
        std::unique_ptr<ASTNode> where;   
    };

    struct DeleteStmt : ASTNode {
        std::string tableName;
        std::unique_ptr<ASTNode> where;
    };

    struct UpdateStmt : ASTNode 
    {
        std::string tableName;
        std::vector<std::pair<std::string, Value>> setClause;
        std::unique_ptr<ASTNode> where;
    };

    struct Expression : ASTNode 
    {
        enum Type { EQ, GT, LT, GE, LE, NE, AND, OR, LITERAL, COLUMN };
        Type type;
        std::string column;
        Value literal;
        std::unique_ptr<Expression> left;
        std::unique_ptr<Expression> right;

        Expression() : type(COLUMN), literal(0) {}
    };

}