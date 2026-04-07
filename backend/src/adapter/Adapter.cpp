#include "Adapter.h"
#include <sstream>
#include <cmath>

Adapter::Adapter(Database& db) : db_(db) {}

AdaptiveConfig Adapter::getInitialConfig(int userId) {
    auto stats = db_.getUserStats(userId);

    if (stats.totalGames > 0) {
        return adaptDifficulty(userId, stats);
    }

    // Новая игра - начинаем со среднего уровня
    return {
        .gridSize = 4,
        .shuffleMoves = 100,
        .difficulty = DifficultyLevel::Medium,
        .enableHints = true,
        .timeLimit = 0
    };
}

AdaptiveConfig Adapter::adaptDifficulty(int userId, const Database::UserStats& stats) {
    DifficultyLevel level = calculateDifficulty(stats.avgEfficiency, stats.totalGames);
    int gridSize = suggestGridSize(level, stats.avgTime);
    int shuffleMoves = suggestShuffleMoves(level);

    bool enableHints = (level == DifficultyLevel::VeryEasy || level == DifficultyLevel::Easy);
    int timeLimit = (level == DifficultyLevel::VeryHard) ? 300 : 0;

    return {
        .gridSize = gridSize,
        .shuffleMoves = shuffleMoves,
        .difficulty = level,
        .enableHints = enableHints,
        .timeLimit = timeLimit
    };
}

DifficultyLevel Adapter::calculateDifficulty(double efficiency, int totalGames) {
    // Учитываем количество сыгранных игр для надёжности оценки
    double confidence = std::min(1.0, static_cast<double>(totalGames) / 10.0);
    efficiency *= confidence;

    if (efficiency > 0.8) {
        return DifficultyLevel::VeryHard;
    }
    else if (efficiency > 0.6) {
        return DifficultyLevel::Hard;
    }
    else if (efficiency > 0.4) {
        return DifficultyLevel::Medium;
    }
    else if (efficiency > 0.2) {
        return DifficultyLevel::Easy;
    }
    else {
        return DifficultyLevel::VeryEasy;
    }
}

int Adapter::suggestGridSize(DifficultyLevel level, double avgTime) {
    switch (level) {
    case DifficultyLevel::VeryEasy:
        return 3;  // 3x3
    case DifficultyLevel::Easy:
        return (avgTime < 120) ? 4 : 3;
    case DifficultyLevel::Medium:
        return 4;  // 4x4
    case DifficultyLevel::Hard:
        return (avgTime < 300) ? 5 : 4;
    case DifficultyLevel::VeryHard:
        return 5;  // 5x5
    default:
        return 4;
    }
}

int Adapter::suggestShuffleMoves(DifficultyLevel level) {
    switch (level) {
    case DifficultyLevel::VeryEasy:
        return 50;
    case DifficultyLevel::Easy:
        return 100;
    case DifficultyLevel::Medium:
        return 150;
    case DifficultyLevel::Hard:
        return 200;
    case DifficultyLevel::VeryHard:
        return 300;
    default:
        return 100;
    }
}

std::string Adapter::getRecommendation(int userId, const Database::UserStats& stats) {
    std::ostringstream oss;

    if (stats.totalGames == 0) {
        return "Добро пожаловать! Начните с лёгкого уровня. Эффективность рассчитывается как: (оптимальные ходы / ваши ходы) × штраф за ошибки.";
    }

    auto config = adaptDifficulty(userId, stats);

    oss << "Ваш уровень: " << difficultyToString(config.difficulty) << ". ";

    // Объясняем расчёт эффективности
    oss << "Эффективность = (оптимальные ходы / ваши ходы) × (1 - ошибки) × (время). ";

    if (stats.avgEfficiency > 0.7) {
        oss << "Отличная работа! Попробуйте увеличить размер поля.";
    }
    else if (stats.avgEfficiency < 0.3) {
        oss << "Рекомендуем снизить сложность и включить подсказки.";
    }
    else if (stats.avgTime > 300) {
        oss << "Попробуйте решать быстрее для лучшего результата.";
    }
    else {
        oss << "Продолжайте в том же духе!";
    }

    return oss.str();
}

std::string Adapter::difficultyToString(DifficultyLevel level) {
    switch (level) {
    case DifficultyLevel::VeryEasy:  return "Очень легко";
    case DifficultyLevel::Easy:      return "Легко";
    case DifficultyLevel::Medium:    return "Средний";
    case DifficultyLevel::Hard:      return "Сложный";
    case DifficultyLevel::VeryHard:  return "Очень сложный";
    default:                         return "Неизвестно";
    }
}

DifficultyLevel Adapter::stringToDifficulty(const std::string& str) {
    if (str == "VeryEasy") return DifficultyLevel::VeryEasy;
    if (str == "Easy") return DifficultyLevel::Easy;
    if (str == "Medium") return DifficultyLevel::Medium;
    if (str == "Hard") return DifficultyLevel::Hard;
    if (str == "VeryHard") return DifficultyLevel::VeryHard;
    return DifficultyLevel::Medium;
}


// подсказки
Hint Adapter::getHint(int userId, const std::vector<std::vector<int>>& board) {
    Hint hint;
    int size = board.size();

    // 1. Находим пустую клетку
    int emptyRow = -1, emptyCol = -1;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (board[r][c] == 0) {
                emptyRow = r;
                emptyCol = c;
                break;
            }
        }
        if (emptyRow != -1) break;
    }

    // 2. СТРАТЕГИЯ: решаем ПОРЯДОВО
    // Сначала ряд 0 (плитки 1,2,3,4), потом ряд 1 (5,6,7,8) и т.д.
    // Для каждого ряда: сначала левую плитку, потом следующую

    // Находим какую плитку нужно ставить следующей
    int targetValue = -1;
    int targetRow = -1, targetCol = -1;

    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            // Последняя клетка должна быть пустой
            if (r == size - 1 && c == size - 1) {
                if (board[r][c] != 0) {
                    targetValue = 0;  // Нужна пустая клетка
                    targetRow = r;
                    targetCol = c;
                }
                continue;
            }

            int expected = r * size + c + 1;
            if (board[r][c] != expected) {
                targetValue = expected;
                targetRow = r;
                targetCol = c;
                break;
            }
        }
        if (targetValue != -1) break;
    }

    // Если всё на местах - победа!
    if (targetValue == -1) {
        hint.description = "Pobeda! Vse plitki na mestakh!";
        return hint;
    }

    // 3. Если нам нужна пустая клетка в конце - просто двигаем что угодно
    if (targetValue == 0) {
        int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
        for (auto& d : dirs) {
            int nr = emptyRow + d[0], nc = emptyCol + d[1];
            if (nr >= 0 && nr < size && nc >= 0 && nc < size && board[nr][nc] != 0) {
                hint = { nr, nc, emptyRow, emptyCol, "Перемести плитку " + std::to_string(board[nr][nc]) };
                return hint;
            }
        }
    }

    // 4. Находим где сейчас целевая плитка
    int tileRow = -1, tileCol = -1;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (board[r][c] == targetValue) {
                tileRow = r;
                tileCol = c;
                break;
            }
        }
        if (tileRow != -1) break;
    }

    // 5. Определяем приоритет движений
    // Приоритет: сначала по вертикали к цели, потом по горизонтали
    std::vector<std::pair<int, int>> priorities;

    if (tileRow < targetRow) priorities.push_back({ 1, 0 });   // вниз
    if (tileRow > targetRow) priorities.push_back({ -1, 0 });  // вверх
    if (tileCol < targetCol) priorities.push_back({ 0, 1 });   // вправо
    if (tileCol > targetCol) priorities.push_back({ 0, -1 });  // влево

    // 6. Проверяем возможные ходы (только соседние с пустой!)
    int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };

    // Сначала ищем ходы, которые приближают целевую плитку
    for (auto& pref : priorities) {
        for (auto& d : dirs) {
            int checkRow = emptyRow + d[0];
            int checkCol = emptyCol + d[1];

            if (checkRow < 0 || checkRow >= size || checkCol < 0 || checkCol >= size)
                continue;

            int val = board[checkRow][checkCol];
            if (val == 0) continue;

            // Проверяем, не нарушим ли уже собранные ряды
            bool breaksSolved = false;
            for (int r = 0; r < size && !breaksSolved; ++r) {
                for (int c = 0; c < size; ++c) {
                    int cellVal = board[r][c];
                    if (cellVal == 0) continue;

                    int expected = r * size + c + 1;
                    // Если плитка на месте И мы её двигаем И она из уже собранных
                    if (cellVal == expected && r == checkRow && c == checkCol) {
                        if (r < targetRow || (r == targetRow && c < targetCol)) {
                            breaksSolved = true;
                            break;
                        }
                    }
                }
            }

            if (breaksSolved) continue;

            // Если это целевая плитка И мы двигаем её в правильном направлении
            if (val == targetValue) {
                int newRow = emptyRow;
                int newCol = emptyCol;

                // Проверяем, приближается ли она к цели
                int oldDist = std::abs(checkRow - targetRow) + std::abs(checkCol - targetCol);
                int newDist = std::abs(newRow - targetRow) + std::abs(newCol - targetCol);

                if (newDist < oldDist) {
                    hint = { checkRow, checkCol, emptyRow, emptyCol,
                           "Отлично! Перемести плитку " + std::to_string(val) + " k celi" };
                    return hint;
                }
            }
        }
    }

    // 7. Если не нашли идеальный ход - ищем любой допустимый
    for (auto& d : dirs) {
        int checkRow = emptyRow + d[0];
        int checkCol = emptyCol + d[1];

        if (checkRow < 0 || checkRow >= size || checkCol < 0 || checkCol >= size)
            continue;

        int val = board[checkRow][checkCol];
        if (val == 0) continue;

        // Проверяем, не нарушаем ли собранные
        bool breaksSolved = false;
        for (int r = 0; r < size && !breaksSolved; ++r) {
            for (int c = 0; c < size; ++c) {
                int cellVal = board[r][c];
                if (cellVal == 0) continue;

                int expected = r * size + c + 1;
                if (cellVal == expected && r == checkRow && c == checkCol) {
                    if (r < targetRow || (r == targetRow && c < targetCol)) {
                        breaksSolved = true;
                        break;
                    }
                }
            }
        }

        if (!breaksSolved) {
            hint = { checkRow, checkCol, emptyRow, emptyCol,
                   "Переместите плитку " + std::to_string(val) };
            return hint;
        }
    }

    // 8. Если совсем ничего не нашли - любую соседнюю
    for (auto& d : dirs) {
        int checkRow = emptyRow + d[0];
        int checkCol = emptyCol + d[1];

        if (checkRow >= 0 && checkRow < size && checkCol >= 0 && checkCol < size) {
            int val = board[checkRow][checkCol];
            if (val != 0) {
                hint = { checkRow, checkCol, emptyRow, emptyCol,
                       "Peremestite plitku " + std::to_string(val) };
                return hint;
            }
        }
    }

    hint.description = "Net vozmozhnyh hodov";
    return hint;
}

// Новая функция для определения уровня мастерства
std::string Adapter::getMasteryLevel(double efficiency, int totalGames) {
    if (totalGames < 3) return "Новичок";

    if (efficiency > 0.8) return "Мастер";
    if (efficiency > 0.6) return "Эксперт";
    if (efficiency > 0.4) return "Продвинутый";
    if (efficiency > 0.2) return "Средний";
    return "Новичок";
}