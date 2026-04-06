#pragma once

#include "Puzzle.h"
#include <string>
#include <chrono>

struct GameStats {
    int sessionId;
    int userId;
    int gridSize;
    int moveCount;
    double timeSpent;
    int optimalMoves;
    double efficiencyScore;
    bool isCompleted;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;

    // Новые метрики для адаптации
    int backAndForthMoves;
    std::vector<std::pair<int, int>> moveHistory;
    double patternScore;
};

class Game {
public:
    Game(int sessionId, int userId, int gridSize = 4);

    void start();
    bool makeMove(int row, int col);
    void end();

    // Геттеры
    int getSessionId() const { return sessionId_; }
    int getUserId() const { return userId_; }
    const Puzzle& getPuzzle() const { return puzzle_; }
    const GameStats& getStats() const { return stats_; }

    // Расчёт эффективности
    double calculateEfficiency() const;

private:
    int sessionId_;
    int userId_;
    Puzzle puzzle_;
    GameStats stats_;
    bool isActive_;
};