#pragma once
#include "Catalog.h"
#include "StorageManager.h"
#include "SQLParser.h"
#include "ResultSet.h"
#include "Expression.h"
#include "log.h"
#include "Table.h"
#include <memory>
#include <unordered_map>

namespace FoxSQL
{
    class Database
    {
    public:
        Database()
        {
            Logger::instance().init();
            LOG_INFO("Database starting...");
            Catalog::instance().init(storage_);
            LOG_INFO("Database initialized.");
        }

        ResultSet executeSQL(const std::string& sql) {
            try {
                LOG_INFO("Executing SQL: " + sql);
                auto stmt = SQLParser::parse(sql);
                return executeStatement(stmt.get());
            }
            catch (const std::exception& e) {
                LOG_ERR("SQL execution failed: " + std::string(e.what()));
                throw;
            }
        }

        // 获取表（如果不存在则创建并缓存）
        Table* getTable(const std::string& name) {
            auto it = tables_.find(name);
            if (it != tables_.end())
                return it->second.get();

            if (!Catalog::instance().tableExists(name))
                throw SQLException("Table not found: " + name);

            auto columns = Catalog::instance().getTableColumns(name);
            auto table = std::make_unique<Table>(name, columns, storage_);
            Table* ptr = table.get();
            tables_[name] = std::move(table);
            return ptr;
        }

        // 保存所有表到磁盘（exit 时调用）
        void save() {
            for (auto& p : tables_) {
                p.second->persistToDisk();
            }
            storage_.flushAll();
            LOG_INFO("All tables saved to disk.");
        }

    private:
        StorageManager storage_;
        std::unordered_map<std::string, std::unique_ptr<Table>> tables_;

        ResultSet executeStatement(const ASTNode* stmt) {
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

        ResultSet executeCreateTable(const CreateTableStmt* stmt) {
            Catalog::instance().createTable(stmt->tableName, stmt->columns);
            getTable(stmt->tableName); // 创建表并缓存
            return ResultSet();
        }

        ResultSet executeInsert(const InsertStmt* stmt) {
            Table* table = getTable(stmt->tableName);
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
            Table* table = getTable(stmt->tableName);
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
            Table* table = getTable(stmt->tableName);
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
            Table* table = getTable(stmt->tableName);
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