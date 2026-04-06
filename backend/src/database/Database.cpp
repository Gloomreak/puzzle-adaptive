#include "Database.h"
#include <iostream>
#include <sstream>

Database::Database(const std::string& dbPath)
    : db_(nullptr), dbPath_(dbPath) {
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::init() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    return createTables();
}

bool Database::createTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            grid_size INTEGER NOT NULL,
            move_count INTEGER NOT NULL,
            time_spent REAL NOT NULL,
            optimal_moves INTEGER NOT NULL,
            efficiency_score REAL NOT NULL,
            is_completed INTEGER NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
        
        CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);
        CREATE INDEX IF NOT EXISTS idx_sessions_timestamp ON sessions(timestamp);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

int Database::createUser(const std::string& username) {
    const char* sql = "INSERT INTO users (username) VALUES (?); SELECT last_insert_rowid();";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    int userId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return userId;
}

bool Database::saveSession(const UserSession& session) {
    const char* sql = R"(
        INSERT INTO sessions 
        (user_id, grid_size, move_count, time_spent, optimal_moves, efficiency_score, is_completed)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, session.userId);
    sqlite3_bind_int(stmt, 2, session.gridSize);
    sqlite3_bind_int(stmt, 3, session.moveCount);
    sqlite3_bind_double(stmt, 4, session.timeSpent);
    sqlite3_bind_int(stmt, 5, session.optimalMoves);
    sqlite3_bind_double(stmt, 6, session.efficiencyScore);
    sqlite3_bind_int(stmt, 7, session.isCompleted ? 1 : 0);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "SQL step error: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    std::cout << "Session saved: user=" << session.userId
        << ", moves=" << session.moveCount
        << ", time=" << session.timeSpent
        << ", efficiency=" << session.efficiencyScore << std::endl;

    return true;
}

std::vector<UserSession> Database::getUserSessions(int userId, int limit) {
    std::vector<UserSession> sessions;

    const char* sql = R"(
        SELECT id, user_id, grid_size, move_count, time_spent, 
               optimal_moves, efficiency_score, is_completed, timestamp
        FROM sessions
        WHERE user_id = ?
        ORDER BY timestamp DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sessions;
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserSession session;
        session.id = sqlite3_column_int(stmt, 0);
        session.userId = sqlite3_column_int(stmt, 1);
        session.gridSize = sqlite3_column_int(stmt, 2);
        session.moveCount = sqlite3_column_int(stmt, 3);
        session.timeSpent = sqlite3_column_double(stmt, 4);
        session.optimalMoves = sqlite3_column_int(stmt, 5);
        session.efficiencyScore = sqlite3_column_double(stmt, 6);
        session.isCompleted = (sqlite3_column_int(stmt, 7) == 1);
        session.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        sessions.push_back(session);
    }

    sqlite3_finalize(stmt);
    return sessions;
}

double Database::getUserAverageEfficiency(int userId) {
    const char* sql = R"(
        SELECT AVG(efficiency_score) 
        FROM sessions 
        WHERE user_id = ? AND is_completed = 1
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0.0;
    }

    sqlite3_bind_int(stmt, 1, userId);

    double avgEfficiency = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        avgEfficiency = sqlite3_column_double(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return avgEfficiency;
}

int Database::getUserTotalGames(int userId) {
    const char* sql = "SELECT COUNT(*) FROM sessions WHERE user_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, userId);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

Database::UserStats Database::getUserStats(int userId) {
    UserStats stats;
    stats.avgEfficiency = getUserAverageEfficiency(userId);
    stats.totalGames = getUserTotalGames(userId);
    stats.avgTime = 0.0;
    stats.completedGames = 0;

    // Дополнительная статистика
    const char* sql = R"(
        SELECT AVG(time_spent), 
               SUM(CASE WHEN is_completed = 1 THEN 1 ELSE 0 END),
               GROUP_CONCAT(grid_size)
        FROM sessions
        WHERE user_id = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.avgTime = sqlite3_column_double(stmt, 0);
            stats.completedGames = sqlite3_column_int(stmt, 1);

            // Parse grid size history
            const char* gridHistory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            if (gridHistory) {
                std::istringstream iss(gridHistory);
                std::string token;
                while (std::getline(iss, token, ',')) {
                    stats.gridSizeHistory.push_back(std::stoi(token));
                }
            }
        }

        sqlite3_finalize(stmt);
    }

    return stats;
}