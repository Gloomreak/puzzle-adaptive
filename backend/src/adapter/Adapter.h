#pragma once

#include "database/Database.h"
#include <vector>

// Выносим Hint вне класса
struct Hint {
    int fromRow = -1;
    int fromCol = -1;
    int toRow = -1;
    int toCol = -1;
    std::string description = "";
};

enum class DifficultyLevel {
    VeryEasy,
    Easy,
    Medium,
    Hard,
    VeryHard
};

struct AdaptiveConfig {
    int gridSize;
    int shuffleMoves;
    DifficultyLevel difficulty;
    bool enableHints;
    int timeLimit;
};

class Adapter {
public:
    Adapter(Database& db);

    AdaptiveConfig getInitialConfig(int userId);
    AdaptiveConfig adaptDifficulty(int userId, const Database::UserStats& stats);
    std::string getRecommendation(int userId, const Database::UserStats& stats);

    // Подсказка
    Hint getHint(int userId, const std::vector<std::vector<int>>& board);

    static std::string getMasteryLevel(double efficiency, int totalGames);

    static std::string difficultyToString(DifficultyLevel level);
    static DifficultyLevel stringToDifficulty(const std::string& str);

private:
    Database& db_;

    DifficultyLevel calculateDifficulty(double efficiency, int totalGames);
    int suggestGridSize(DifficultyLevel level, double avgTime);
    int suggestShuffleMoves(DifficultyLevel level);
};

