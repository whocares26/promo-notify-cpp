#pragma once
#include <sqlite3.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace core {

class Database {
 public:
    explicit Database(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error(
                std::string("Failed to open DB: ") + sqlite3_errmsg(db_));
        }
    }

    ~Database() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void execute(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "unknown error";
            sqlite3_free(err);
            throw std::runtime_error("DB execute error: " + msg);
        }
    }

    using RowCallback = std::function<void(sqlite3_stmt*)>;

    void query(const std::string& sql,
               const std::vector<std::string>& params,
               RowCallback callback) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(
                std::string("DB prepare error: ") + sqlite3_errmsg(db_));
        }
        for (int i = 0; i < static_cast<int>(params.size()); ++i) {
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            callback(stmt);
        }
        sqlite3_finalize(stmt);
    }

    int64_t insert(const std::string& sql,
                   const std::vector<std::string>& params) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(
                std::string("DB prepare error: ") + sqlite3_errmsg(db_));
        }
        for (int i = 0; i < static_cast<int>(params.size()); ++i) {
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return sqlite3_last_insert_rowid(db_);
    }

 private:
    sqlite3* db = nullptr;
};

}  // namespace core
