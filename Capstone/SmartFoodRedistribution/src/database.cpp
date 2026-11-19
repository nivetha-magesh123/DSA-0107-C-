#include "database.hpp"
#include <iostream>
#include <stdexcept>

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string message = db_ ? sqlite3_errmsg(db_) : "Could not open SQLite database";
        if (db_) sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error(message);
    }
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

bool Database::execute(const std::string& sql) {
    char* error = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: " << (error ? error : "unknown") << std::endl;
        sqlite3_free(error);
        return false;
    }
    return true;
}

void Database::initialize() {
    const char* schema = R"SQL(
    CREATE TABLE IF NOT EXISTS donors (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        phone TEXT NOT NULL,
        email TEXT DEFAULT '',
        location TEXT NOT NULL,
        donor_type TEXT DEFAULT 'Individual',
        created_at TEXT DEFAULT (datetime('now','localtime'))
    );
    CREATE TABLE IF NOT EXISTS receivers (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        phone TEXT DEFAULT '',
        location TEXT NOT NULL,
        urgency INTEGER NOT NULL CHECK(urgency BETWEEN 1 AND 10),
        distance_km REAL NOT NULL CHECK(distance_km >= 0),
        required_quantity INTEGER NOT NULL CHECK(required_quantity > 0),
        people_count INTEGER DEFAULT 0,
        created_at TEXT DEFAULT (datetime('now','localtime'))
    );
    CREATE TABLE IF NOT EXISTS food_donations (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        donor_id INTEGER NOT NULL,
        food_name TEXT NOT NULL,
        food_type TEXT NOT NULL,
        quantity INTEGER NOT NULL CHECK(quantity > 0),
        prepared_time TEXT DEFAULT '',
        expiry_time TEXT NOT NULL,
        pickup_location TEXT NOT NULL,
        status TEXT NOT NULL DEFAULT 'Available',
        created_at TEXT DEFAULT (datetime('now','localtime')),
        FOREIGN KEY(donor_id) REFERENCES donors(id)
    );
    CREATE TABLE IF NOT EXISTS distributions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        food_id INTEGER NOT NULL,
        receiver_id INTEGER NOT NULL,
        allocated_quantity INTEGER NOT NULL,
        status TEXT NOT NULL DEFAULT 'Allocated',
        allocation_date TEXT DEFAULT (datetime('now','localtime')),
        updated_at TEXT DEFAULT (datetime('now','localtime')),
        FOREIGN KEY(food_id) REFERENCES food_donations(id),
        FOREIGN KEY(receiver_id) REFERENCES receivers(id)
    );
    )SQL";
    if (!execute(schema)) throw std::runtime_error("Could not create database tables");
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: if (c >= 0x20) out += static_cast<char>(c);
        }
    }
    return out;
}
