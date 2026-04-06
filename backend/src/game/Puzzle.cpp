#include "Puzzle.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <cmath>

Puzzle::Puzzle(int size)
    : size_(size), moveCount_(0) {
    board_.resize(size_, std::vector<int>(size_));
    init();
}

void Puzzle::init() {
    // Создаём решённое состояние
    int num = 1;
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            if (i == size_ - 1 && j == size_ - 1) {
                board_[i][j] = 0;  // Пустая клетка
                emptyRow_ = i;
                emptyCol_ = j;
            }
            else {
                board_[i][j] = num++;
            }
        }
    }
    moveCount_ = 0;
}

void Puzzle::shuffle(int moves) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dirDist(0, 3);

    // Делаем случайные ходы от решённого состояния
    // Это ГАРАНТИРУЕТ решаемость!
    for (int i = 0; i < moves; ++i) {
        int dir = dirDist(gen);
        int newRow = emptyRow_, newCol = emptyCol_;

        // 0: вверх, 1: вниз, 2: влево, 3: вправо
        switch (dir) {
        case 0: if (emptyRow_ > 0) newRow--; break;
        case 1: if (emptyRow_ < size_ - 1) newRow++; break;
        case 2: if (emptyCol_ > 0) newCol--; break;
        case 3: if (emptyCol_ < size_ - 1) newCol++; break;
        }

        // Делаем ход только если клетка соседняя
        if (newRow != emptyRow_ || newCol != emptyCol_) {
            swap(emptyRow_, emptyCol_, newRow, newCol);
            emptyRow_ = newRow;
            emptyCol_ = newCol;
        }
    }

    moveCount_ = 0;
}

bool Puzzle::canMove(int row, int col) const {
    if (row < 0 || row >= size_ || col < 0 || col >= size_)
        return false;

    // Можно двигать только плитку, соседнюю с пустой
    int rowDiff = std::abs(row - emptyRow_);
    int colDiff = std::abs(col - emptyCol_);

    return (rowDiff + colDiff == 1);
}

bool Puzzle::move(int row, int col) {
    if (!canMove(row, col))
        return false;

    swap(row, col, emptyRow_, emptyCol_);
    emptyRow_ = row;
    emptyCol_ = col;
    moveCount_++;

    return true;
}

void Puzzle::swap(int row1, int col1, int row2, int col2) {
    std::swap(board_[row1][col1], board_[row2][col2]);
}

bool Puzzle::isSolved() const {
    int num = 1;
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            if (i == size_ - 1 && j == size_ - 1) {
                if (board_[i][j] != 0)
                    return false;
            }
            else {
                if (board_[i][j] != num++)
                    return false;
            }
        }
    }
    return true;
}

int Puzzle::calculateOptimalMoves() const {
    // Эвристика Манхэттена (приближённая оценка)
    int moves = 0;
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            int val = board_[i][j];
            if (val == 0) continue;

            int targetRow = (val - 1) / size_;
            int targetCol = (val - 1) % size_;

            moves += std::abs(i - targetRow) + std::abs(j - targetCol);
        }
    }
    return moves;
}

std::string Puzzle::toJson() const {
    std::ostringstream oss;
    oss << "{\"size\":" << size_
        << ",\"board\":[";

    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            oss << board_[i][j];
            if (!(i == size_ - 1 && j == size_ - 1))
                oss << ",";
        }
    }

    oss << "],\"moves\":" << moveCount_
        << ",\"solved\":" << (isSolved() ? "true" : "false")
        << "}";

    return oss.str();
}