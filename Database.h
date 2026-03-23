#pragma once
#include "Catalog.h"
#include "StorageManager.h"
#include "Executor.h"
#include "SQLParser.h"
#include "ResultSet.h"
#include "log.h"
#include <memory>

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
                Executor exec(storage_);
                ResultSet result = exec.execute(stmt.get());
                LOG_INFO("SQL executed successfully.");
                return result;
            }
            catch (const std::exception& e) {
                LOG_ERR("SQL execution failed: " + std::string(e.what()));
                throw;
            }
        }

        std::unique_ptr<Table> openTable(const std::string& name) {
            auto columns = Catalog::instance().getTableColumns(name);
            return std::make_unique<Table>(name, columns, storage_);
        }

        void shutdown() {
            storage_.flushAll();
            LOG_INFO("Database shut down.");
        }

    private:
        StorageManager storage_;
    };

} // namespace FoxSQL