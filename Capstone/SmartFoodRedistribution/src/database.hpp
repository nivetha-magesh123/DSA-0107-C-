#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool execute(const std::string& sql);
    sqlite3* handle() const { return db_; }
    void initialize();
private:
    sqlite3* db_ = nullptr;
};

std::string jsonEscape(const std::string& value);
