#include "Game.h"
#include <iostream>

Game::Game(int sessionId, int userId, int gridSize)
    : sessionId_(sessionId)
    , userId_(userId)
    , puzzle_(gridSize)
    , isActive_(false) {

    stats_.sessionId = sessionId;
    stats_.userId = userId;
    stats_.gridSize = gridSize;
    stats_.moveCount = 0;
    stats_.timeSpent = 0.0;
    stats_.optimalMoves = 0;
    stats_.efficiencyScore = 0.0;
    stats_.isCompleted = false;
    stats_.backAndForthMoves = 0;
    stats_.moveHistory.clear();
    stats_.patternScore = 0.0;
}

void Game::start(int shuffleMoves) {
    puzzle_.init();
    puzzle_.shuffle(shuffleMoves);  // Перемешиваем указанным числом ходов
    stats_.startTime = std::chrono::system_clock::now();
    stats_.optimalMoves = puzzle_.calculateOptimalMoves();
    stats_.moveCount = 0;
    stats_.backAndForthMoves = 0;
    stats_.moveHistory.clear();
    isActive_ = true;
}

bool Game::makeMove(int row, int col) {
    if (!isActive_)
        return false;

    bool success = puzzle_.move(row, col);
    if (success) {
        stats_.moveCount = puzzle_.getMoveCount();

        // Проверяем на ход туда-сюда: если текущий ход совпадает с предыдущим — считаем это возвратом
        if (!stats_.moveHistory.empty()) {
            auto lastMove = stats_.moveHistory.back();
            if (lastMove.first == row && lastMove.second == col) {
                stats_.backAndForthMoves++;
            }
        }

        stats_.moveHistory.push_back({ row, col });

        if (puzzle_.isSolved()) {
            end();
        }
    }

    return success;
}

void Game::end() {
    stats_.endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration<double>(stats_.endTime - stats_.startTime);
    stats_.timeSpent = duration.count();
    stats_.isCompleted = true;
    // Не пересчитываем optimalMoves при завершении — оно вычисляется при старте и сохраняется
    stats_.efficiencyScore = calculateEfficiency();
    isActive_ = false;

    std::cout << "Game ended: moves=" << stats_.moveCount
        << ", time=" << stats_.timeSpent
        << ", optimal=" << stats_.optimalMoves
        << ", efficiency=" << stats_.efficiencyScore << std::endl;
}


double Game::calculateEfficiency() const {
    if (stats_.optimalMoves == 0 || stats_.moveCount == 0)
        return 0.0;

    // Базовая эффективность
    double baseEfficiency = static_cast<double>(stats_.optimalMoves) / stats_.moveCount;

    // Штраф за ошибки (ходы туда-сюда)
    double errorPenalty = 1.0;
    if (stats_.moveCount > 0) {
        double errorRate = static_cast<double>(stats_.backAndForthMoves) / stats_.moveCount;
        errorPenalty = 1.0 - (errorRate * 0.5);  // До 50% штрафа
    }

    // Штраф за время
    double timePenalty = 1.0;
    double maxTime = stats_.gridSize * 60.0;  // 1 минута на размер поля
    if (stats_.timeSpent > maxTime) {
        timePenalty = maxTime / stats_.timeSpent;
    }

    return baseEfficiency * errorPenalty * timePenalty;
}