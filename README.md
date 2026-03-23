# FoxSQL__0.1
Upgraded version of B-tree based on version 0.0
================================================================================
FoxSQL 0.1 – Lightweight SQL Database Engine
================================================================================

This document provides an overview of the FoxSQL source files and explains the
purpose of each main class and function.

-------------------------------------------------------------------------------
1. common.h
-------------------------------------------------------------------------------
Defines common types and exceptions.

- enum class ColumnType { INT, VARCHAR }
  Data types supported by the engine.

- struct ColumnMeta
  Metadata for a table column: name, type, max length, primary key flag.

- class SQLException : public std::runtime_error
  Exception thrown for SQL errors (parse errors, duplicate key, table not found).

- using byte = unsigned char
  Alias for raw byte data.

-------------------------------------------------------------------------------
2. Types.h
-------------------------------------------------------------------------------
Constants for B+ tree configuration.

- constexpr size_t B_PLUS_TREE_ORDER = 4
  Order of the B+ tree (number of children per node).

- constexpr size_t B_PLUS_TREE_MAX_KEYS = B_PLUS_TREE_ORDER - 1
  Maximum keys per node.

- constexpr size_t B_PLUS_TREE_MIN_KEYS = (B_PLUS_TREE_ORDER / 2) - 1
  Minimum keys per node (except root).

- using Key = int64_t
  Type used for index keys (here, the primary key value).

-------------------------------------------------------------------------------
3. fs.h
-------------------------------------------------------------------------------
File system helper functions (Windows API).

- bool createDirectory(const std::string& path)
  Creates a single directory.

- bool createDirectories(const std::string& path)
  Creates all missing directories in a path.

- bool fileExists(const std::string& path)
  Checks if a file exists.

- bool directoryExists(const std::string& path)
  Checks if a directory exists.

- std::string getDataDir()
  Returns the default data directory "C:\FoxOS\Data\".

- void initDataDir()
  Creates the default data directory if missing.

-------------------------------------------------------------------------------
4. log.h
-------------------------------------------------------------------------------
Simple thread‑safe logger.

- class Logger (singleton)
  - void init(const std::string& filename = "foxsql.log")
    Opens the log file (appending).
  - void log(Level level, const std::string& msg)
    Writes a timestamped message to the log file and to stdout.
- Macros: LOG_INFO, LOG_WARN, LOG_ERR
  Convenience macros to log with levels.

-------------------------------------------------------------------------------
5. Config.h
-------------------------------------------------------------------------------
Global configuration (data directory, buffer pool size).

- class Config (singleton)
  - void setDataDir(const std::string& dir)
    Changes the data directory.
  - std::string getDataDir()
    Returns the current data directory.
  - void setBufferPoolSize(size_t size)
    Sets the buffer pool capacity.
  - size_t getBufferPoolSize()
    Returns the buffer pool capacity.

-------------------------------------------------------------------------------
6. Page.h
-------------------------------------------------------------------------------
Represents a fixed‑size disk page.

- constexpr size_t PAGE_SIZE = 4096
  Size of a page in bytes.

- class Page
  - Page(size_t id)
    Constructs a page with given ID, initialised to zero.
  - void* getData()  returns the raw data pointer.
  - void readFrom(const void* src, size_t offset, size_t len)
    Copies data into the page at the given offset; marks page dirty.
  - void writeTo(void* dst, size_t offset, size_t len) const
    Copies data from the page to the destination.
  - bool isDirty() / markDirty() / clearDirty()
    Manage the dirty flag for write‑back.

-------------------------------------------------------------------------------
7. BufferPool.h
-------------------------------------------------------------------------------
LRU buffer pool for caching pages.

- class BufferPool
  - Page* fetchPage(size_t pageId)
    Retrieves a page; loads it from disk if not in cache, evicts if necessary.
  - void markDirty(Page* page)
    Marks the page as dirty so it will be written back later.
  - void flushAll()
    Writes all dirty pages back to disk (placeholder).

-------------------------------------------------------------------------------
8. StorageManager.h
-------------------------------------------------------------------------------
Handles table file operations and record‑level I/O.

- class StorageManager
  - void createTableFile(const std::string& tableName)
    Creates a new .ft file for a table (with header).
  - void writeRecord(const std::string& tableName, size_t rid, const std::vector<byte>& data)
    Writes a record to the file at the page identified by `rid`.
  - std::vector<byte> readRecord(const std::string& tableName, size_t rid)
    Reads a record from the file.
  - void deleteRecord(const std::string& tableName, size_t rid)
    Marks a record as deleted by writing a zero length.
  - void flushAll()
    Flushes all file streams.

-------------------------------------------------------------------------------
9. BPlusTree.h
-------------------------------------------------------------------------------
B+ tree index implementation (in‑memory, used for primary keys).

- template <typename K, typename V> struct BPlusTreeNode
  Node structure with keys, values (for leaves), children, and next pointer.

- template <typename K, typename V> class BPlusTree
  - void insert(const K& key, const V& value)
    Inserts a key‑value pair.
  - V lookup(const K& key) const
    Returns the value associated with the key, or throws if not found.
  - void remove(const K& key)
    Deletes the key from the tree.
  - void clear()
    Deletes all nodes.
  The class also contains internal methods for splitting, borrowing, merging.

-------------------------------------------------------------------------------
10. IndexManager.h
-------------------------------------------------------------------------------
Wrapper to manage multiple named indices (currently only "primary").

- class IndexManager
  - template <typename K, typename V> void createIndex(const std::string& name)
    Creates a new B+ tree index.
  - template <typename K, typename V> void insert(const std::string& name, const K& key, const V& value)
    Inserts into the named index.
  - template <typename K, typename V> V lookup(const std::string& name, const K& key) const
    Looks up in the named index.
  - template <typename K, typename V> void remove(const std::string& name, const K& key)
    Removes from the named index.
  - void clear()
    Deletes all indices.

-------------------------------------------------------------------------------
11. Row.h
-------------------------------------------------------------------------------
Value and Row classes for in‑memory data representation.

- struct Value
  Holds a value of type INT or VARCHAR using a union.
  - Value()                  default constructor (zero).
  - Value(int64_t val)       construct from int.
  - Value(std::string val)   construct from string.
  - int64_t getInt() const   retrieves int (throws if type mismatch).
  - const std::string& getString() const
  - Copy and move constructors / assignment operators.

- class Row
  Stores a mapping from column name to Value.
  - void setValue(const std::string& col, Value val)
  - const Value& getValue(const std::string& col) const
  - bool hasColumn(const std::string& col) const

-------------------------------------------------------------------------------
12. RecordFormat.h
-------------------------------------------------------------------------------
Serialization and deserialization of Row objects to/from byte buffers.

- class RecordFormat
  - static std::vector<byte> serialize(const Row& row, const std::vector<ColumnMeta>& columns)
    Encodes a row into a byte vector according to the column definitions.
  - static Row deserialize(const byte* data, const std::vector<ColumnMeta>& columns)
    Decodes a byte buffer back to a Row.
  - static size_t getRecordSize(...) computes the size of a serialized record.

-------------------------------------------------------------------------------
13. AST.h
-------------------------------------------------------------------------------
Abstract Syntax Tree nodes for parsed SQL statements.

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

-------------------------------------------------------------------------------
14. SQLParser.h
-------------------------------------------------------------------------------
Lexer and recursive‑descent parser that produces an AST.

- class SQLParser
  - static std::unique_ptr<ASTNode> parse(const std::string& sql)
    Main entry point.
  The internal class Parser performs tokenisation and grammar rules.

-------------------------------------------------------------------------------
15. Expression.h
-------------------------------------------------------------------------------
Evaluator for WHERE conditions.

- class ExpressionEvaluator
  - static bool evaluate(const Expression* expr, const Row& row, const std::vector<ColumnMeta>& columns)
    Evaluates the expression on the given row.
  Internally it uses getValue to obtain column values or literals.

-------------------------------------------------------------------------------
16. Catalog.h
-------------------------------------------------------------------------------
Manages table schemas and persists them to disk.

- class Catalog (singleton)
  - void init(StorageManager& storage)
    Initialises the catalog, loads from catalog.ft.
  - void createTable(const std::string& name, const std::vector<ColumnMeta>& columns)
    Adds a table and persists the catalog.
  - const std::vector<ColumnMeta>& getTableColumns(const std::string& name) const
    Returns the schema of a table.
  - bool tableExists(const std::string& name) const
    Checks if a table exists.
  - std::vector<std::string> getAllTableNames() const
    Lists all tables.
  The class also contains private methods loadFromFile() and persistAllTables().

-------------------------------------------------------------------------------
17. Table.h
-------------------------------------------------------------------------------
Represents an actual table: manages records, primary key index, RID list.

- class Table
  - Table(const std::string& name, const std::vector<ColumnMeta>& columns, StorageManager& storage)
    Constructs a table; loads existing data from file if any.
  - size_t insertRow(const Row& row)
    Inserts a row; checks duplicate key; writes to storage; updates index and RID list.
  - Row getRowByPrimaryKey(int64_t key) const
    Retrieves a row using the index.
  - void deleteRowByPrimaryKey(int64_t key)
    Deletes the row; marks record as deleted; removes from index and RID list.
  - void updateRowByPrimaryKey(int64_t key, const Row& newRow)
    Updates the row (keeps same RID).
  - const std::vector<size_t>& getAllRids() const
    Returns the list of all valid record IDs (used for full scan).
  - void loadMetadata()
    Reads the table file, collects valid RIDs, and rebuilds the index.
  - size_t allocateRID()
    Allocates a new record ID (page number) and adds it to rids_.

-------------------------------------------------------------------------------
18. Executor.h
-------------------------------------------------------------------------------
Executes an AST statement, performing the actual database operations.

- class Executor
  - ResultSet execute(const ASTNode* stmt)
    Dispatches to the appropriate statement handler.
  - executeCreateTable, executeInsert, executeSelect, executeDelete, executeUpdate
    Each creates a Table object and performs the action; SELECT uses table.getAllRids() for full scan.
  - Helper methods: readRecordByRID, evaluateWhere, projectColumns, getPrimaryKeyValue.

-------------------------------------------------------------------------------
19. ResultSet.h
-------------------------------------------------------------------------------
Container for query results.

- class ResultSet
  - void addRow(Row row)
    Adds a row.
  - iterator begin() / end()
    For iteration.
  - size_t size() / bool empty()
    Query size.

-------------------------------------------------------------------------------
20. help.h
-------------------------------------------------------------------------------
Displays help text in the console.

- class Help
  - static void show()
    Prints supported SQL syntax and special commands.

-------------------------------------------------------------------------------
21. main.cpp
-------------------------------------------------------------------------------
Entry point of the program. Sets up console title, displays welcome banner,
and runs an interactive command loop.

- Functions:
  - void SetColor(int color) / ResetColor()
    Windows console colour control.
  - void HighlightPrint(const std::string& text)
    Prints SQL with syntax highlighting (keywords blue, strings red, numbers green).
  - int main()
    The main loop: prompts "foxsql> ", reads a line, handles help/quit, executes SQL,
    and displays results or errors.

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
The data directory is "C:\FoxOS\Data\".  Ensure the program has write permission.
Log file "foxsql.log" is also stored there.

For any questions or contributions, please refer to the project homepage.
