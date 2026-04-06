#pragma once

#include <string>
#include <vector>
#include <sqlite3.h>


struct UserSession {
    int id;
    int userId;
    int gridSize;
    int moveCount;
    double timeSpent;
    int optimalMoves;
    double efficiencyScore;
    bool isCompleted;
    std::string timestamp;
};

class Database {
public:
    Database(const std::string& dbPath = "puzzle.db");
    ~Database();

    // Инициализация
    bool init();

    // Пользователи
    int createUser(const std::string& username = "anonymous");

    // Сессии
    bool saveSession(const UserSession& session);
    std::vector<UserSession> getUserSessions(int userId, int limit = 10);

    // Статистика
    double getUserAverageEfficiency(int userId);
    int getUserTotalGames(int userId);

    // Аналитика для адаптации
    struct UserStats {
        double avgEfficiency;
        double avgTime;
        int totalGames;
        int completedGames;
        std::vector<int> gridSizeHistory;
    };
    UserStats getUserStats(int userId);

private:
    sqlite3* db_;
    std::string dbPath_;

    bool createTables();
};