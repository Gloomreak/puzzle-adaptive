#include "Adapter.h"
#include <sstream>
#include <cmath>
#include <algorithm>

Adapter::Adapter(Database& db) : db_(db) {}

AdaptiveConfig Adapter::getInitialConfig(int userId) {
    auto stats = db_.getUserStats(userId);

    if (stats.totalGames > 0) {
        return adaptDifficulty(userId, stats);
    }

    // Новая игра - начинаем со среднего уровня
    return AdaptiveConfig{ 4, 100, DifficultyLevel::Medium, true, 0 };
}

AdaptiveConfig Adapter::adaptDifficulty(int userId, const Database::UserStats& stats) {
    DifficultyLevel level = calculateDifficulty(stats.avgEfficiency, stats.totalGames);
    int gridSize = suggestGridSize(level, stats.avgTime);
    int shuffleMoves = suggestShuffleMoves(level);

    // Включаем подсказки для уровней до и включая Medium
    bool enableHints = (level == DifficultyLevel::VeryEasy || level == DifficultyLevel::Easy || level == DifficultyLevel::Medium);
    int timeLimit = (level == DifficultyLevel::VeryHard) ? 300 : 0;

    return AdaptiveConfig{ gridSize, shuffleMoves, level, enableHints, timeLimit };
}

DifficultyLevel Adapter::calculateDifficulty(double efficiency, int totalGames) {
    // Стабилизация оценки при небольшом числе игр:
    // используем blend между текущей эффективностью и нейтральным значением 0.5
    double confidence = std::min(1.0, static_cast<double>(totalGames) / 10.0);
    double blended = efficiency * (0.5 + 0.5 * confidence) + 0.5 * (1.0 - confidence);

    // Пороговые значения с >=, чтобы избежать "провалов" на границах
    if (blended >= 0.85) {
        return DifficultyLevel::VeryHard;
    }
    else if (blended >= 0.7) {
        return DifficultyLevel::Hard;
    }
    else if (blended >= 0.45) {
        return DifficultyLevel::Medium;
    }
    else if (blended >= 0.25) {
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
        return "Добро пожаловать! Начните с лёгкого уровня. Эффективность рассчитывается как (оптимальные ходы / ваши ходы) × (1 - ошибки).";
    }

    auto config = adaptDifficulty(userId, stats);

    oss << "Ваш уровень: " << difficultyToString(config.difficulty) << ". ";

    // Объясняем расчёт эффективности кратко и по-русски
    oss << "Эффективность = оптимальные ходы ÷ ваши ходы (с учётом ошибок). ";

    if (stats.avgEfficiency >= 0.75) {
        oss << "Отличная работа! Попробуйте увеличить размер поля.";
    }
    else if (stats.avgEfficiency < 0.35) {
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
    case DifficultyLevel::VeryEasy:  return "Очень лёгкий";
    case DifficultyLevel::Easy:      return "Лёгкий";
    case DifficultyLevel::Medium:    return "Средний";
    case DifficultyLevel::Hard:      return "Сложный";
    case DifficultyLevel::VeryHard:  return "Очень сложный";
    default:                         return "Неизвестно";
    }
}

DifficultyLevel Adapter::stringToDifficulty(const std::string& str) {
    // Принимаем как английские, так и русские обозначения
    if (str == "VeryEasy" || str == "Very Easy" || str == "Очень лёгкий" || str == "Очень легкий") return DifficultyLevel::VeryEasy;
    if (str == "Easy" || str == "Лёгкий" || str == "Легкий") return DifficultyLevel::Easy;
    if (str == "Medium" || str == "Средний") return DifficultyLevel::Medium;
    if (str == "Hard" || str == "Сложный") return DifficultyLevel::Hard;
    if (str == "VeryHard" || str == "Very Hard" || str == "Очень сложный") return DifficultyLevel::VeryHard;
    return DifficultyLevel::Medium;
}

// подсказки
Hint Adapter::getHint(int userId, const std::vector<std::vector<int>>& board) {
    Hint hint;
    int size = static_cast<int>(board.size());

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

    if (emptyRow == -1) {
        hint.description = "Пустой клетки не найдено";
        return hint;
    }

    // 2. Стратегия: ищем первую неверную позицию в порядке обхода по строкам
    int targetValue = -1;
    int targetRow = -1, targetCol = -1;

    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (r == size - 1 && c == size - 1) {
                if (board[r][c] != 0) {
                    targetValue = 0; // хотим пустую в конце
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

    // Всё на местах
    if (targetValue == -1) {
        hint.description = "Победа! Все плитки на местах!";
        return hint;
    }

    // Если нужно освободить последнюю клетку
    if (targetValue == 0) {
        int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
        for (auto& d : dirs) {
            int nr = emptyRow + d[0], nc = emptyCol + d[1];
            if (nr >= 0 && nr < size && nc >= 0 && nc < size && board[nr][nc] != 0) {
                hint = { nr, nc, emptyRow, emptyCol, "Переместите плитку " + std::to_string(board[nr][nc]) };
                return hint;
            }
        }
    }

    // 3. Рассматриваем только соседние с пустой клетки — это единственно допустимые ходы
    std::vector<std::pair<int, int>> neighbors;
    int dirsAll[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
    for (int i = 0; i < 4; ++i) {
        int nr = emptyRow + dirsAll[i][0];
        int nc = emptyCol + dirsAll[i][1];
        if (nr >= 0 && nr < size && nc >= 0 && nc < size) {
            if (board[nr][nc] != 0) neighbors.emplace_back(nr, nc);
        }
    }

    // 4. Находим, где сейчас целевая плитка
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

    // 5. Оцениваем кандидатов — выбираем один лучший соседний ход
    int bestFromR = -1, bestFromC = -1;
    int bestToR = -1, bestToC = -1;
    int bestDelta = -100000; // максимальное улучшение (oldDist - newDist)
    bool foundTargetTileCandidate = false;

    for (auto& p : neighbors) {
        int r = p.first, c = p.second;
        int val = board[r][c];
        if (val == 0) continue;

        // Проверяем, не нарушим ли уже собранные позиции, если двинем эту плитку
        bool breaksSolved = false;
        for (int rr = 0; rr < size && !breaksSolved; ++rr) {
            for (int cc = 0; cc < size; ++cc) {
                int cellVal = board[rr][cc];
                if (cellVal == 0) continue;
                int expected = rr * size + cc + 1;
                if (cellVal == expected && rr == r && cc == c) {
                    if (rr < targetRow || (rr == targetRow && cc < targetCol)) {
                        breaksSolved = true;
                        break;
                    }
                }
            }
        }
        if (breaksSolved) continue;

        // Если это целевая плитка — приоритет
        int oldDist = std::abs(r - targetRow) + std::abs(c - targetCol);
        int newDist = std::abs(emptyRow - targetRow) + std::abs(emptyCol - targetCol);
        int delta = oldDist - newDist; // положительное — улучшение

        if (val == targetValue) {
            // Сильный приоритет: сразу возвращаем, если улучшает
            if (delta > 0) {
                hint = { r, c, emptyRow, emptyCol, "Отлично! Переместите плитку " + std::to_string(val) + " к цели" };
                return hint;
            }
            else {
                // пометим как кандидат, но не сразу возвращаем
                foundTargetTileCandidate = true;
                if (delta > bestDelta) {
                    bestDelta = delta;
                    bestFromR = r; bestFromC = c;
                    bestToR = emptyRow; bestToC = emptyCol;
                }
            }
        }
        else {
            // Обычный кандидат
            if (delta > bestDelta) {
                bestDelta = delta;
                bestFromR = r; bestFromC = c;
                bestToR = emptyRow; bestToC = emptyCol;
            }
        }
    }

    // 6. Если нашли лучший кандидат с улучшением — вернуть его
    if (bestDelta > 0 && bestFromR != -1) {
        hint = { bestFromR, bestFromC, bestToR, bestToC, "Переместите плитку " + std::to_string(board[bestFromR][bestFromC]) };
        return hint;
    }

    // 7. Если был кандидат-целевой (даже без улучшения) — вернуть его
    if (foundTargetTileCandidate && bestFromR != -1) {
        hint = { bestFromR, bestFromC, bestToR, bestToC, "Переместите плитку " + std::to_string(board[bestFromR][bestFromC]) };
        return hint;
    }

    // 8. В крайнем случае — вернуть любой соседний допустимый ход
    if (!neighbors.empty()) {
        auto p = neighbors.front();
        hint = { p.first, p.second, emptyRow, emptyCol, "Переместите плитку " + std::to_string(board[p.first][p.second]) };
        return hint;
    }

    hint.description = "Нет возможных ходов";
    return hint;
}

// Новая функция для определения уровня мастерства
std::string Adapter::getMasteryLevel(double efficiency, int totalGames) {
    if (totalGames < 3) return "Новичок";

    if (efficiency >= 0.85) return "Мастер";
    if (efficiency >= 0.7) return "Эксперт";
    if (efficiency >= 0.45) return "Продвинутый";
    if (efficiency >= 0.25) return "Средний";
    return "Новичок";
}