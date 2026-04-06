#pragma once

#include <vector>
#include <string>
#include <optional>

class Puzzle {
public:
    Puzzle(int size = 4);  // По умолчанию 4x4 (классические пятнашки)

    // Инициализация игры
    void init();
    void shuffle(int moves = 100);  // Перемешивание

    // Игровая механика
    bool move(int row, int col);  // Попытка переместить плитку
    bool isSolved() const;        // Проверка на решение

    // Геттеры
    int getSize() const { return size_; }
    const std::vector<std::vector<int>>& getBoard() const { return board_; }
    int getMoveCount() const { return moveCount_; }

    // Сериализация для JSON (для отправки на клиент)
    std::string toJson() const;

    // Оптимальное решение (для оценки сложности)
    int calculateOptimalMoves() const;

private:
    int size_;
    std::vector<std::vector<int>> board_;
    int emptyRow_, emptyCol_;  // Позиция пустой клетки
    int moveCount_;

    bool canMove(int row, int col) const;
    void swap(int row1, int col1, int row2, int col2);
    bool isSolvable() const;
};