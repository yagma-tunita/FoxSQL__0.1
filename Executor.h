#pragma once
#include "AST.h"
#include "Catalog.h"
#include "StorageManager.h"
#include "Table.h"
#include "ResultSet.h"
#include "Expression.h"
#include "SQLParser.h"

namespace FoxSQL {

    class Executor {
    public:
        Executor(StorageManager& storage) : storage_(storage) {}

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
        StorageManager& storage_;

        ResultSet executeCreateTable(const CreateTableStmt* stmt) {
            Catalog::instance().createTable(stmt->tableName, stmt->columns);
            Table table(stmt->tableName, stmt->columns, storage_);
            return ResultSet();
        }

        ResultSet executeInsert(const InsertStmt* stmt) {
            auto columns = Catalog::instance().getTableColumns(stmt->tableName);
            Table table(stmt->tableName, columns, storage_);

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
                    const std::string& colName = pair.first;
                    const Value& val = pair.second;
                    row.setValue(colName, val);
                }
            }
            table.insertRow(row);
            return ResultSet();
        }

        ResultSet executeSelect(const SelectStmt* stmt) {
            auto columns = Catalog::instance().getTableColumns(stmt->tableName);
            Table table(stmt->tableName, columns, storage_);
            ResultSet result;
            for (size_t rid : table.getAllRids()) {
                try {
                    Row row = readRecordByRID(stmt->tableName, rid, columns);
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
            auto columns = Catalog::instance().getTableColumns(stmt->tableName);
            Table table(stmt->tableName, columns, storage_);
            for (size_t rid : table.getAllRids()) {
                try {
                    Row row = readRecordByRID(stmt->tableName, rid, columns);
                    const Expression* whereExpr = dynamic_cast<const Expression*>(stmt->where.get());
                    if (evaluateWhere(whereExpr, row, columns)) {
                        int64_t key = getPrimaryKeyValue(row, columns);
                        table.deleteRowByPrimaryKey(key);
                    }
                }
                catch (const SQLException&) {}
            }
            return ResultSet();
        }

        ResultSet executeUpdate(const UpdateStmt* stmt) 
        {
            auto columns = Catalog::instance().getTableColumns(stmt->tableName);
            Table table(stmt->tableName, columns, storage_);
            for (size_t rid : table.getAllRids()) 
            {
                try 
                {
                    Row row = readRecordByRID(stmt->tableName, rid, columns);
                    const Expression* whereExpr = dynamic_cast<const Expression*>(stmt->where.get());
                    if (evaluateWhere(whereExpr, row, columns))
                    {
                        for (const auto& pair : stmt->setClause) 
                        {
                            const std::string& colName = pair.first;
                            const Value& val = pair.second;
                            row.setValue(colName, val);
                        }
                        int64_t key = getPrimaryKeyValue(row, columns);
                        table.updateRowByPrimaryKey(key, row);
                    }
                }
                catch (const SQLException&) {}
            }
            return ResultSet();
        }

        Row readRecordByRID(const std::string& tableName, size_t rid,
            const std::vector<ColumnMeta>& columns) const {
            auto data = storage_.readRecord(tableName, rid);
            return RecordFormat::deserialize(data.data(), columns);
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

        int64_t getPrimaryKeyValue(const Row& row, const std::vector<ColumnMeta>& columns) const {
            for (const auto& col : columns) {
                if (col.is_primary) {
                    return row.getValue(col.name).getInt();
                }
            }
            throw SQLException("No primary key");
        }
    };

}