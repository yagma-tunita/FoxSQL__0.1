#pragma once
#include "AST.h"
#include "Catalog.h"
#include "Database.h"
#include "Table.h"
#include "ResultSet.h"
#include "Expression.h"
#include "SQLParser.h"

namespace FoxSQL {

    class Executor {
    public:
        Executor(Database& db) : db_(db) {}

        ResultSet execute(const ASTNode* stmt) {
            if (auto create = dynamic_cast<const CreateTableStmt*>(stmt)) {
                return executeCreateTable(create);
            }
            else if (auto insert = dynamic_cast<const InsertStmt*>(stmt)) {
                return executeInsert(insert);
            }
            else if (auto select = dynamic_cast<const SelectStmt*>(stmt)) {
                return executeSelect(select);
            }
            else if (auto del = dynamic_cast<const DeleteStmt*>(stmt)) {
                return executeDelete(del);
            }
            else if (auto update = dynamic_cast<const UpdateStmt*>(stmt)) {
                return executeUpdate(update);
            }
            throw SQLException("Unknown statement");
        }

    private:
        Database& db_;

        ResultSet executeCreateTable(const CreateTableStmt* stmt) {
            Catalog::instance().createTable(stmt->tableName, stmt->columns);
            // 创建表后立即从缓存获取（或新建）
            db_.getTable(stmt->tableName);
            return ResultSet();
        }

        ResultSet executeInsert(const InsertStmt* stmt) {
            Table* table = db_.getTable(stmt->tableName);
            const auto& columns = table->getColumns();

            Row row;
            if (stmt->values.empty() || stmt->values[0].first.empty()) {
                if (stmt->values.size() != columns.size())
                    throw SQLException("Number of values does not match number of columns");
                for (size_t i = 0; i < columns.size(); ++i) {
                    row.setValue(columns[i].name, stmt->values[i].second);
                }
            }
            else {
                for (const auto& pair : stmt->values) {
                    row.setValue(pair.first, pair.second);
                }
            }
            table->insertRow(row);
            return ResultSet();
        }

        ResultSet executeSelect(const SelectStmt* stmt) {
            Table* table = db_.getTable(stmt->tableName);
            const auto& columns = table->getColumns();
            ResultSet result;

            for (int64_t key : table->getAllKeys()) {
                try {
                    Row row = table->getRowByPrimaryKey(key);
                    const Expression* whereExpr = dynamic_cast<const Expression*>(stmt->where.get());
                    if (evaluateWhere(whereExpr, row, columns)) {
                        Row projected = projectColumns(row, stmt->columns, columns);
                        result.addRow(std::move(projected));
                    }
                }
                catch (const SQLException&) {}
            }
            return result;
        }

        ResultSet executeDelete(const DeleteStmt* stmt) {
            Table* table = db_.getTable(stmt->tableName);
            const auto& columns = table->getColumns();

            for (int64_t key : table->getAllKeys()) {
                try {
                    Row row = table->getRowByPrimaryKey(key);
                    const Expression* whereExpr = dynamic_cast<const Expression*>(stmt->where.get());
                    if (evaluateWhere(whereExpr, row, columns)) {
                        table->deleteRowByPrimaryKey(key);
                    }
                }
                catch (const SQLException&) {}
            }
            return ResultSet();
        }

        ResultSet executeUpdate(const UpdateStmt* stmt) {
            Table* table = db_.getTable(stmt->tableName);
            const auto& columns = table->getColumns();

            for (int64_t key : table->getAllKeys()) {
                try {
                    Row row = table->getRowByPrimaryKey(key);
                    const Expression* whereExpr = dynamic_cast<const Expression*>(stmt->where.get());
                    if (evaluateWhere(whereExpr, row, columns)) {
                        for (const auto& pair : stmt->setClause) {
                            row.setValue(pair.first, pair.second);
                        }
                        table->updateRowByPrimaryKey(key, row);
                    }
                }
                catch (const SQLException&) {}
            }
            return ResultSet();
        }

        bool evaluateWhere(const Expression* expr, const Row& row,
            const std::vector<ColumnMeta>& columns) const {
            return ExpressionEvaluator::evaluate(expr, row, columns);
        }

        Row projectColumns(const Row& row, const std::vector<std::string>& colNames,
            const std::vector<ColumnMeta>& allColumns) const {
            Row result;
            if (colNames.empty()) {
                for (const auto& col : allColumns) {
                    result.setValue(col.name, row.getValue(col.name));
                }
            }
            else {
                for (const auto& name : colNames) {
                    result.setValue(name, row.getValue(name));
                }
            }
            return result;
        }
    };
}