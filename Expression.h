#pragma once
#include "AST.h"
#include "Row.h"
#include "common.h"

namespace FoxSQL {

    class ExpressionEvaluator 
    {
    public:
        static bool evaluate(const Expression* expr, const Row& row, const std::vector<ColumnMeta>& columns) 
        {
            if (!expr) return true;
            switch (expr->type) {
            case Expression::EQ: {
                Value left = getValue(expr->left.get(), row, columns);
                Value right = getValue(expr->right.get(), row, columns);
                if (left.type != right.type) return false;
                if (left.type == Value::Type::INT) return left.getInt() == right.getInt();
                else return left.getString() == right.getString();
            }
            case Expression::GT: {
                Value left = getValue(expr->left.get(), row, columns);
                Value right = getValue(expr->right.get(), row, columns);
                if (left.type != right.type) return false;
                if (left.type == Value::Type::INT) return left.getInt() > right.getInt();
                else return left.getString() > right.getString();
            }
            case Expression::LT: {
                Value left = getValue(expr->left.get(), row, columns);
                Value right = getValue(expr->right.get(), row, columns);
                if (left.type != right.type) return false;
                if (left.type == Value::Type::INT) return left.getInt() < right.getInt();
                else return left.getString() < right.getString();
            }
            case Expression::GE: {
                Value left = getValue(expr->left.get(), row, columns);
                Value right = getValue(expr->right.get(), row, columns);
                if (left.type != right.type) return false;
                if (left.type == Value::Type::INT) return left.getInt() >= right.getInt();
                else return left.getString() >= right.getString();
            }
            case Expression::LE: {
                Value left = getValue(expr->left.get(), row, columns);
                Value right = getValue(expr->right.get(), row, columns);
                if (left.type != right.type) return false;
                if (left.type == Value::Type::INT) return left.getInt() <= right.getInt();
                else return left.getString() <= right.getString();
            }
            case Expression::NE: {
                Value left = getValue(expr->left.get(), row, columns);
                Value right = getValue(expr->right.get(), row, columns);
                if (left.type != right.type) return true;
                if (left.type == Value::Type::INT) return left.getInt() != right.getInt();
                else return left.getString() != right.getString();
            }
            case Expression::AND:
                return evaluate(expr->left.get(), row, columns) && evaluate(expr->right.get(), row, columns);
            case Expression::OR:
                return evaluate(expr->left.get(), row, columns) || evaluate(expr->right.get(), row, columns);
            default:
                throw SQLException("Unsupported expression type");
            }
        }

    private:
        static Value getValue(const Expression* expr, const Row& row, const std::vector<ColumnMeta>& columns) {
            if (expr->type == Expression::COLUMN) {
                return row.getValue(expr->column);
            }
            else if (expr->type == Expression::LITERAL) {
                return expr->literal;
            }
            throw SQLException("Invalid expression for value extraction");
        }
    };

}