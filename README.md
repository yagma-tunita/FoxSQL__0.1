# FoxSQL__0.1
Upgraded version of B-tree based on version 0.0
================================================================================
FoxSQL 0.1 – Lightweight SQL Database Engine
轻量级 SQL 数据库引擎
================================================================================

This document provides an overview of the FoxSQL source files and explains the
purpose of each main class and function.
本文档概述 FoxSQL 源代码文件，并解释每个主要类与函数的作用。

-------------------------------------------------------------------------------
1. common.h
-------------------------------------------------------------------------------
Defines common types and exceptions.
定义公共类型与异常。

- enum class ColumnType { INT, VARCHAR }
  Data types supported by the engine.
  引擎支持的数据类型。

- struct ColumnMeta
  Metadata for a table column: name, type, max length, primary key flag.
  表列的元数据：名称、类型、最大长度、主键标志。

- class SQLException : public std::runtime_error
  Exception thrown for SQL errors (parse errors, duplicate key, table not found).
  SQL 错误时抛出的异常（解析错误、主键重复、表不存在等）。

- using byte = unsigned char
  Alias for raw byte data.
  原始字节数据的别名。

-------------------------------------------------------------------------------
2. Types.h
-------------------------------------------------------------------------------
Constants for B+ tree configuration.
B+ 树配置常量。

- constexpr size_t B_PLUS_TREE_ORDER = 4
  Order of the B+ tree (number of children per node).
  B+ 树的阶数（每个节点的最大子节点数）。

- constexpr size_t B_PLUS_TREE_MAX_KEYS = B_PLUS_TREE_ORDER - 1
  Maximum keys per node.
  每个节点最大键数量。

- constexpr size_t B_PLUS_TREE_MIN_KEYS = (B_PLUS_TREE_ORDER / 2) - 1
  Minimum keys per node (except root).
  每个节点最小键数量（根节点除外）。

- using Key = int64_t
  Type used for index keys (here, the primary key value).
  索引键的类型（此处为主键值）。

-------------------------------------------------------------------------------
3. fs.h
-------------------------------------------------------------------------------
File system helper functions (Windows API).
文件系统辅助函数（Windows API）。

- bool createDirectory(const std::string& path)
  Creates a single directory. 创建单个目录。

- bool createDirectories(const std::string& path)
  Creates all missing directories in a path.
  创建路径中所有缺失的目录。

- bool fileExists(const std::string& path)
  Checks if a file exists. 检查文件是否存在。

- bool directoryExists(const std::string& path)
  Checks if a directory exists. 检查目录是否存在。

- std::string getDataDir()
  Returns the default data directory "C:\FoxOS\Data\".
  返回默认数据目录 "C:\FoxOS\Data\"。

- void initDataDir()
  Creates the default data directory if missing.
  如果缺失则创建默认数据目录。

-------------------------------------------------------------------------------
4. log.h
-------------------------------------------------------------------------------
Simple thread‑safe logger.
简单的线程安全日志记录器。

- class Logger (singleton)
  - void init(const std::string& filename = "foxsql.log")
    Opens the log file (appending). 打开日志文件（追加模式）。
  - void log(Level level, const std::string& msg)
    Writes a timestamped message to the log file and to stdout.
    将带时间戳的消息写入日志文件和标准输出。
- Macros: LOG_INFO, LOG_WARN, LOG_ERR
  Convenience macros to log with levels.
  按级别记录日志的便捷宏。

-------------------------------------------------------------------------------
5. Config.h
-------------------------------------------------------------------------------
Global configuration (data directory, buffer pool size).
全局配置（数据目录、缓冲池大小）。

- class Config (singleton)
  - void setDataDir(const std::string& dir)
    Changes the data directory. 更改数据目录。
  - std::string getDataDir()
    Returns the current data directory. 返回当前数据目录。
  - void setBufferPoolSize(size_t size)
    Sets the buffer pool capacity. 设置缓冲池容量。
  - size_t getBufferPoolSize()
    Returns the buffer pool capacity. 返回缓冲池容量。

-------------------------------------------------------------------------------
6. Page.h
-------------------------------------------------------------------------------
Represents a fixed‑size disk page.
表示固定大小的磁盘页面。

- constexpr size_t PAGE_SIZE = 4096
  Size of a page in bytes. 页面大小（字节）。

- class Page
  - Page(size_t id)
    Constructs a page with given ID, initialised to zero.
    用给定 ID 构造页面，初始化为零。
  - void* getData()  returns the raw data pointer.
    返回原始数据指针。
  - void readFrom(const void* src, size_t offset, size_t len)
    Copies data into the page at the given offset; marks page dirty.
    将数据复制到页面的指定偏移处；标记页面为脏。
  - void writeTo(void* dst, size_t offset, size_t len) const
    Copies data from the page to the destination.
    将页面中的数据复制到目标缓冲区。
  - bool isDirty() / markDirty() / clearDirty()
    Manage the dirty flag for write‑back.
    管理写回所需的脏标志。

-------------------------------------------------------------------------------
7. BufferPool.h
-------------------------------------------------------------------------------
LRU buffer pool for caching pages.
LRU 缓冲池，用于缓存页面。

- class BufferPool
  - Page* fetchPage(size_t pageId)
    Retrieves a page; loads it from disk if not in cache, evicts if necessary.
    获取页面；若不在缓存中则从磁盘加载，必要时淘汰页面。
  - void markDirty(Page* page)
    Marks the page as dirty so it will be written back later.
    将页面标记为脏，以便稍后写回。
  - void flushAll()
    Writes all dirty pages back to disk (placeholder).
    将所有脏页写回磁盘（占位实现）。

-------------------------------------------------------------------------------
8. StorageManager.h
-------------------------------------------------------------------------------
Handles table file operations and record‑level I/O.
处理表文件操作和记录级 I/O。

- class StorageManager
  - void createTableFile(const std::string& tableName)
    Creates a new .ft file for a table (with header).
    为表创建新的 .ft 文件（含文件头）。
  - void writeRecord(const std::string& tableName, size_t rid, const std::vector<byte>& data)
    Writes a record to the file at the page identified by `rid`.
    将记录写入由 `rid` 标识的页面。
  - std::vector<byte> readRecord(const std::string& tableName, size_t rid)
    Reads a record from the file. 从文件中读取记录。
  - void deleteRecord(const std::string& tableName, size_t rid)
    Marks a record as deleted by writing a zero length.
    通过写入零长度将记录标记为已删除。
  - void flushAll()
    Flushes all file streams. 刷新所有文件流。

-------------------------------------------------------------------------------
9. BPlusTree.h
-------------------------------------------------------------------------------
B+ tree index implementation (in‑memory, used for primary keys).
B+ 树索引实现（内存中，用于主键）。

- template <typename K, typename V> struct BPlusTreeNode
  Node structure with keys, values (for leaves), children, and next pointer.
  节点结构，包含键、值（叶子节点）、子节点和下一个指针。

- template <typename K, typename V> class BPlusTree
  - void insert(const K& key, const V& value)
    Inserts a key‑value pair. 插入键值对。
  - V lookup(const K& key) const
    Returns the value associated with the key, or throws if not found.
    返回键对应的值，若未找到则抛出异常。
  - void remove(const K& key)
    Deletes the key from the tree. 从树中删除键。
  - void clear()
    Deletes all nodes. 删除所有节点。
  The class also contains internal methods for splitting, borrowing, merging.
  类中还包含分裂、借位、合并等内部方法。

-------------------------------------------------------------------------------
10. IndexManager.h
-------------------------------------------------------------------------------
Wrapper to manage multiple named indices (currently only "primary").
管理多个命名索引的包装器（当前仅支持 "primary"）。

- class IndexManager
  - template <typename K, typename V> void createIndex(const std::string& name)
    Creates a new B+ tree index. 创建新的 B+ 树索引。
  - template <typename K, typename V> void insert(const std::string& name, const K& key, const V& value)
    Inserts into the named index. 插入到指定索引。
  - template <typename K, typename V> V lookup(const std::string& name, const K& key) const
    Looks up in the named index. 在指定索引中查找。
  - template <typename K, typename V> void remove(const std::string& name, const K& key)
    Removes from the named index. 从指定索引中删除。
  - void clear()
    Deletes all indices. 删除所有索引。

-------------------------------------------------------------------------------
11. Row.h
-------------------------------------------------------------------------------
Value and Row classes for in‑memory data representation.
内存数据表示的 Value 和 Row 类。

- struct Value
  Holds a value of type INT or VARCHAR using a union.
  使用联合体保存 INT 或 VARCHAR 类型的值。
  - Value()                  default constructor (zero).
  - Value(int64_t val)       construct from int.
  - Value(std::string val)   construct from string.
  - int64_t getInt() const   retrieves int (throws if type mismatch).
  - const std::string& getString() const
  - Copy and move constructors / assignment operators.
    拷贝/移动构造函数和赋值运算符。

- class Row
  Stores a mapping from column name to Value.
  存储列名到 Value 的映射。
  - void setValue(const std::string& col, Value val)
  - const Value& getValue(const std::string& col) const
  - bool hasColumn(const std::string& col) const

-------------------------------------------------------------------------------
12. RecordFormat.h
-------------------------------------------------------------------------------
Serialization and deserialization of Row objects to/from byte buffers.
Row 对象与字节缓冲区之间的序列化与反序列化。

- class RecordFormat
  - static std::vector<byte> serialize(const Row& row, const std::vector<ColumnMeta>& columns)
    Encodes a row into a byte vector according to the column definitions.
    根据列定义将行编码为字节向量。
  - static Row deserialize(const byte* data, const std::vector<ColumnMeta>& columns)
    Decodes a byte buffer back to a Row. 将字节缓冲区解码为 Row。
  - static size_t getRecordSize(...) computes the size of a serialized record.
    计算序列化记录的大小。

-------------------------------------------------------------------------------
13. AST.h
-------------------------------------------------------------------------------
Abstract Syntax Tree nodes for parsed SQL statements.
解析 SQL 语句的抽象语法树节点。

- struct ASTNode (base)
- struct CreateTableStmt : ASTNode
  tableName, columns
- struct InsertStmt : ASTNode
  tableName, values (list of pair<colName, Value>)
- struct SelectStmt : ASTNode
  tableName, columns (empty means *), where (Expression)
- struct DeleteStmt : ASTNode
  tableName, where
- struct UpdateStmt : ASTNode
  tableName, setClause (list of pair<colName, Value>), where
- struct Expression : ASTNode
  type (EQ, GT, LT, GE, LE, NE, AND, OR, LITERAL, COLUMN)
  column (for COLUMN), literal (for LITERAL), left/right for binary ops.
  类型，列名（COLUMN 时），字面量（LITERAL 时），左右子表达式（二元操作）。

-------------------------------------------------------------------------------
14. SQLParser.h
-------------------------------------------------------------------------------
Lexer and recursive‑descent parser that produces an AST.
词法分析器和递归下降解析器，生成 AST。

- class SQLParser
  - static std::unique_ptr<ASTNode> parse(const std::string& sql)
    Main entry point. 主入口。
  The internal class Parser performs tokenisation and grammar rules.
  内部类 Parser 执行词法分析和语法规则。

-------------------------------------------------------------------------------
15. Expression.h
-------------------------------------------------------------------------------
Evaluator for WHERE conditions.
WHERE 条件的求值器。

- class ExpressionEvaluator
  - static bool evaluate(const Expression* expr, const Row& row, const std::vector<ColumnMeta>& columns)
    Evaluates the expression on the given row.
    在给定行上求值表达式。
  Internally it uses getValue to obtain column values or literals.
  内部使用 getValue 获取列值或字面量。

-------------------------------------------------------------------------------
16. Catalog.h
-------------------------------------------------------------------------------
Manages table schemas and persists them to disk.
管理表模式并将其持久化到磁盘。

- class Catalog (singleton)
  - void init(StorageManager& storage)
    Initialises the catalog, loads from catalog.ft.
    初始化目录，从 catalog.ft 加载。
  - void createTable(const std::string& name, const std::vector<ColumnMeta>& columns)
    Adds a table and persists the catalog.
    添加表并持久化目录。
  - const std::vector<ColumnMeta>& getTableColumns(const std::string& name) const
    Returns the schema of a table. 返回表模式。
  - bool tableExists(const std::string& name) const
    Checks if a table exists. 检查表是否存在。
  - std::vector<std::string> getAllTableNames() const
    Lists all tables. 列出所有表。
  The class also contains private methods loadFromFile() and persistAllTables().
  类中还包含私有方法 loadFromFile() 和 persistAllTables()。

-------------------------------------------------------------------------------
17. Table.h
-------------------------------------------------------------------------------
Represents an actual table: manages records, primary key index, RID list.
表示实际表：管理记录、主键索引、RID 列表。

- class Table
  - Table(const std::string& name, const std::vector<ColumnMeta>& columns, StorageManager& storage)
    Constructs a table; loads existing data from file if any.
    构造表；如果文件存在则加载现有数据。
  - size_t insertRow(const Row& row)
    Inserts a row; checks duplicate key; writes to storage; updates index and RID list.
    插入行；检查主键重复；写入存储；更新索引和 RID 列表。
  - Row getRowByPrimaryKey(int64_t key) const
    Retrieves a row using the index. 使用索引检索行。
  - void deleteRowByPrimaryKey(int64_t key)
    Deletes the row; marks record as deleted; removes from index and RID list.
    删除行；标记记录为删除；从索引和 RID 列表中移除。
  - void updateRowByPrimaryKey(int64_t key, const Row& newRow)
    Updates the row (keeps same RID). 更新行（保持 RID 不变）。
  - const std::vector<size_t>& getAllRids() const
    Returns the list of all valid record IDs (used for full scan).
    返回所有有效记录 ID 的列表（用于全表扫描）。
  - void loadMetadata()
    Reads the table file, collects valid RIDs, and rebuilds the index.
    读取表文件，收集有效 RID，并重建索引。
  - size_t allocateRID()
    Allocates a new record ID (page number) and adds it to rids_.
    分配新的记录 ID（页面号）并添加到 rids_ 中。

-------------------------------------------------------------------------------
18. Executor.h
-------------------------------------------------------------------------------
Executes an AST statement, performing the actual database operations.
执行 AST 语句，完成实际数据库操作。

- class Executor
  - ResultSet execute(const ASTNode* stmt)
    Dispatches to the appropriate statement handler.
    分发给对应的语句处理器。
  - executeCreateTable, executeInsert, executeSelect, executeDelete, executeUpdate
    Each creates a Table object and performs the action; SELECT uses table.getAllRids() for full scan.
    各自创建 Table 对象并执行操作；SELECT 使用 table.getAllRids() 进行全表扫描。
  - Helper methods: readRecordByRID, evaluateWhere, projectColumns, getPrimaryKeyValue.
    辅助方法：通过 RID 读取记录、求值 WHERE 条件、投影列、获取主键值。

-------------------------------------------------------------------------------
19. ResultSet.h
-------------------------------------------------------------------------------
Container for query results.
查询结果的容器。

- class ResultSet
  - void addRow(Row row)
    Adds a row. 添加一行。
  - iterator begin() / end()
    For iteration. 用于迭代。
  - size_t size() / bool empty()
    Query size. 查询大小。

-------------------------------------------------------------------------------
20. help.h
-------------------------------------------------------------------------------
Displays help text in the console.
在控制台显示帮助文本。

- class Help
  - static void show()
    Prints supported SQL syntax and special commands.
    打印支持的 SQL 语法和特殊命令。

-------------------------------------------------------------------------------
21. main.cpp
-------------------------------------------------------------------------------
Entry point of the program. Sets up console title, displays welcome banner,
and runs an interactive command loop.
程序入口。设置控制台标题，显示欢迎横幅，并运行交互式命令循环。

- Functions:
  - void SetColor(int color) / ResetColor()
    Windows console colour control.
    Windows 控制台颜色控制。
  - void HighlightPrint(const std::string& text)
    Prints SQL with syntax highlighting (keywords blue, strings red, numbers green).
    带语法高亮打印 SQL（关键字蓝色，字符串红色，数字绿色）。
  - int main()
    The main loop: prompts "foxsql> ", reads a line, handles help/quit, executes SQL,
    and displays results or errors.
    主循环：提示 "foxsql> "，读取一行，处理 help/quit，执行 SQL，显示结果或错误。

-------------------------------------------------------------------------------
Building and Running
================================================================================

1. Open the solution in Visual Studio (2017 or later).
2. Set the project character set to "Use Unicode Character Set".
3. Build the solution (Ctrl+Shift+B).
4. Run the generated executable.
5. Use SQL statements as described in help.

The data directory is "C:\FoxOS\Data\".  Ensure the program has write permission.
Log file "foxsql.log" is also stored there.

For any questions or contributions, please refer to the project homepage.
