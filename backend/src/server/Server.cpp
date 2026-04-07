#pragma execution_character_set("utf-8")

#include "Server.h"
#include "httplib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <filesystem>
#include "game/Game.h"

namespace fs = std::filesystem;

// Получить путь к директории с exe файлом
std::string getExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    return exePath.substr(0, exePath.find_last_of("\\/"));
}

Server::Server(int port) : port_(port), running_(false) {
    db_ = std::make_unique<Database>("puzzle.db");
    db_->init();
    adapter_ = std::make_unique<Adapter>(*db_);
}

Server::~Server() {
    stop();
}

std::string Server::handleHint() {
    if (games_.empty()) {
        return "{\"success\":false,\"error\":\"No active game\"}";
    }

    auto& game = games_.begin()->second;
    auto board = game->getPuzzle().getBoard();  // Получаем доску
    auto hint = adapter_->getHint(game->getUserId(), board);  // Передаём доску

    std::ostringstream oss;
    oss << "{\"success\":true"
        << ",\"fromRow\":" << hint.fromRow
        << ",\"fromCol\":" << hint.fromCol
        << ",\"toRow\":" << hint.toRow
        << ",\"toCol\":" << hint.toCol
        << ",\"description\":\"" << hint.description << "\"}";

    return oss.str();
}

std::string Server::handleMastery() {
    int userId = 1;
    auto stats = db_->getUserStats(userId);

    std::ostringstream oss;
    oss << "{\"mastery\":\"" << Adapter::getMasteryLevel(stats.avgEfficiency, stats.totalGames) << "\""
        << ",\"efficiency\":" << stats.avgEfficiency
        << ",\"totalGames\":" << stats.totalGames
        << ",\"completedGames\":" << stats.completedGames
        << ",\"explanation\":\"Эффективность = (оптимальные ходы / ваши ходы) * (1 - ошибки) * (время)\"}";

    return oss.str();
}

bool Server::start() {
    httplib::Server svr;

    // Определяем путь к frontend относительно exe
    std::string exeDir = getExecutablePath();
    std::string frontendPath = exeDir + "/frontend";

    std::cout << "Executable path: " << exeDir << std::endl;
    std::cout << "Frontend path: " << frontendPath << std::endl;

    // Главная страница
    svr.Get("/", [&svr, &frontendPath](const httplib::Request&, httplib::Response& res) {
        std::string filePath = frontendPath + "/index.html";
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            res.set_content(content, "text/html; charset=utf-8");
            std::cout << "✓ Отдаём index.html, размер: " << content.size() << " байт" << std::endl;
        }
        else {
            res.status = 404;
            std::string error = "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>File not found: " + filePath + "</h1></body></html>";
            res.set_content(error, "text/html; charset=utf-8");
            std::cout << "✗ Файл не найден: " << filePath << std::endl;
        }
        });

    // CSS файлы
    svr.Get("/css/style.css", [&frontendPath](const httplib::Request&, httplib::Response& res) {
        std::string filePath = frontendPath + "/css/style.css";
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/css; charset=utf-8");
        }
        else {
            res.status = 404;
            res.set_content("/* CSS not found */", "text/css");
        }
        });

    // JavaScript файлы
    svr.Get("/js/app.js", [&frontendPath](const httplib::Request&, httplib::Response& res) {
        std::string filePath = frontendPath + "/js/app.js";
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "application/javascript; charset=utf-8");
        }
        else {
            res.status = 404;
            res.set_content("// JS not found", "application/javascript");
        }
        });

    // API endpoints
    svr.Post("/api/game/create", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(handleCreateGame(req.body), "application/json; charset=utf-8");
        });

    svr.Post("/api/game/move", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(handleMove(req.body), "application/json; charset=utf-8");
        });

    svr.Get("/api/stats", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(handleStats(), "application/json; charset=utf-8");
        });

    svr.Get("/api/config", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(handleConfig(), "application/json; charset=utf-8");
        });

    // API: Получить подсказку
    svr.Get("/api/hint", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(handleHint(), "application/json; charset=utf-8");
        });
    svr.Get("/api/mastery", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(handleMastery(), "application/json; charset=utf-8");
        });

    std::cout << "=== Сервер запускается ===" << std::endl;
    std::cout << "Откройте в браузере: http://localhost:" << port_ << std::endl;
    std::cout << "Нажмите Ctrl+C для остановки" << std::endl;

    running_ = true;
    svr.listen("localhost", port_);
    running_ = false;

    return true;
}

void Server::stop() {
    if (running_) {
        std::cout << "Сервер остановлен" << std::endl;
        running_ = false;
    }
}

std::string Server::handleStaticFile(const std::string& path) {
    std::ifstream file("frontend/" + path);
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "<h1>Файл не найден</h1>";
}

std::string Server::handleCreateGame(const std::string& body) {
    // Парсим необязательные параметры из тела запроса: difficulty (string) и enableHints (bool)
    auto findString = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = body.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        // Пропускаем пробелы
        while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\"')) {
            if (body[pos] == '\"') break;
            pos++;
        }
        if (pos >= body.size() || body[pos] != '\"') {
            // возможно значение без кавычек
            size_t end = body.find_first_of(",}", pos);
            if (end == std::string::npos) return "";
            return body.substr(pos, end - pos);
        }
        pos++; // skip opening quote
        size_t end = body.find('\"', pos);
        if (end == std::string::npos) return "";
        return body.substr(pos, end - pos);
        };

    auto findBool = [&](const std::string& key) -> std::optional<bool> {
        std::string search = "\"" + key + "\":";
        size_t pos = body.find(search);
        if (pos == std::string::npos) return std::nullopt;
        pos += search.length();
        while (pos < body.size() && (body[pos] == ' ')) pos++;
        if (body.compare(pos, 4, "true") == 0) return true;
        if (body.compare(pos, 5, "false") == 0) return false;
        return std::nullopt;
        };

    int userId = 1;
    int sessionId = nextSessionId_++;
    auto config = adapter_->getInitialConfig(userId);

    // Применяем параметры из запроса, если они есть
    std::string diffStr = findString("difficulty");
    if (!diffStr.empty() && diffStr != "auto") {
        // Преобразуем строку сложности в enum
        DifficultyLevel forced = Adapter::stringToDifficulty(diffStr);
        config.difficulty = forced;
        // Простая логика сопоставления gridSize и shuffleMoves с уровнем
        switch (forced) {
        case DifficultyLevel::VeryEasy:  config.gridSize = 3; config.shuffleMoves = 50; break;
        case DifficultyLevel::Easy:      config.gridSize = 4; config.shuffleMoves = 100; break;
        case DifficultyLevel::Medium:    config.gridSize = 4; config.shuffleMoves = 150; break;
        case DifficultyLevel::Hard:      config.gridSize = 5; config.shuffleMoves = 200; break;
        case DifficultyLevel::VeryHard:  config.gridSize = 5; config.shuffleMoves = 300; break;
        default:                         config.gridSize = 4; config.shuffleMoves = 150; break;
        }
    }

    auto hintsOpt = findBool("enableHints");
    if (hintsOpt.has_value()) {
        config.enableHints = hintsOpt.value();
    }

    games_[sessionId] = std::make_unique<Game>(sessionId, userId, config.gridSize);
    games_[sessionId]->start(config.shuffleMoves);

    auto& puzzle = games_[sessionId]->getPuzzle();
    auto board = puzzle.getBoard();

    std::ostringstream oss;
    oss << "{\"success\":true,\"sessionId\":" << sessionId
        << ",\"size\":" << config.gridSize
        << ",\"board\":[";

    for (int i = 0; i < config.gridSize; ++i) {
        for (int j = 0; j < config.gridSize; ++j) {
            oss << board[i][j];
            if (!(i == config.gridSize - 1 && j == config.gridSize - 1))
                oss << ",";
        }
    }

    // Экранируем название сложности и корректно формируем JSON
    std::string difficulty = Adapter::difficultyToString(config.difficulty);
    std::ostringstream diffEsc;
    for (char c : difficulty) {
        switch (c) {
        case '"': diffEsc << "\\\""; break;
        case '\\': diffEsc << "\\\\"; break;
        default: diffEsc << c; break;
        }
    }
    oss << "],\"difficulty\":\"" << diffEsc.str() << "\""
        << ",\"enableHints\":" << (config.enableHints ? "true" : "false") << "}";

    return oss.str();
}

std::string Server::handleMove(const std::string& body) {
    try {
        // Простой парсинг JSON (ищем значения)
        auto findValue = [](const std::string& json, const std::string& key) -> int {
            std::string searchKey = "\"" + key + "\":";
            size_t pos = json.find(searchKey);
            if (pos == std::string::npos) return -1;

            pos += searchKey.length();
            while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

            int value = 0;
            while (pos < json.length() && std::isdigit(json[pos])) {
                value = value * 10 + (json[pos] - '0');
                pos++;
            }
            return value;
            };

        int sessionId = findValue(body, "sessionId");
        int row = findValue(body, "row");
        int col = findValue(body, "col");

        std::cout << "HandleMove: sessionId=" << sessionId << ", row=" << row << ", col=" << col << std::endl;

        if (sessionId < 0 || row < 0 || col < 0) {
            return "{\"success\":false,\"error\":\"Invalid parameters\"}";
        }

        auto gameIt = games_.find(sessionId);
        if (gameIt == games_.end()) {
            std::cout << "Game not found: " << sessionId << std::endl;
            return "{\"success\":false,\"error\":\"Game not found\"}";
        }

        auto& game = gameIt->second;
        bool success = game->makeMove(row, col);
        bool solved = game->getPuzzle().isSolved();

        std::cout << "Move result: success=" << success << ", solved=" << solved << std::endl;

        std::ostringstream oss;
        oss << "{\"success\":" << (success ? "true" : "false")
            << ",\"solved\":" << (solved ? "true" : "false")
            << ",\"moves\":" << game->getStats().moveCount;

        if (success) {
            auto board = game->getPuzzle().getBoard();
            int size = game->getPuzzle().getSize();
            oss << ",\"board\":[";
            bool first = true;
            for (int i = 0; i < size; ++i) {
                for (int j = 0; j < size; ++j) {
                    if (!first) oss << ",";
                    oss << board[i][j];
                    first = false;
                }
            }
            oss << "]";
        }

        if (solved) {
            game->end();
            auto stats = game->getStats();
            UserSession session;
            session.userId = stats.userId;
            session.gridSize = stats.gridSize;
            session.moveCount = stats.moveCount;
            session.timeSpent = stats.timeSpent;
            session.optimalMoves = stats.optimalMoves;
            session.efficiencyScore = stats.efficiencyScore;
            session.isCompleted = true;
            db_->saveSession(session);

            oss << ",\"efficiency\":" << stats.efficiencyScore
                << ",\"message\":\"Congratulations! Puzzle solved!\"";
        }

        oss << "}";
        std::string result = oss.str();
        std::cout << "Response: " << result << std::endl;
        return result;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in handleMove: " << e.what() << std::endl;
        return "{\"success\":false,\"error\":\"Internal server error\"}";
    }
}

std::string Server::handleStats() {
    int userId = 1;
    auto stats = db_->getUserStats(userId);
    auto rec = adapter_->getRecommendation(userId, stats);

    // Экранируем русские символы для JSON
    std::ostringstream oss;
    oss << "{\"totalGames\":" << stats.totalGames
        << ",\"completedGames\":" << stats.completedGames
        << ",\"avgEfficiency\":" << stats.avgEfficiency
        << ",\"avgTime\":" << stats.avgTime
        << ",\"recommendation\":\"";

    // Экранирование строки рекомендации
    for (char c : rec) {
        switch (c) {
        case '"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default: oss << c;
        }
    }
    oss << "\"}";

    return oss.str();
}

std::string Server::handleConfig() {
    int userId = 1;
    auto config = adapter_->getInitialConfig(userId);

    std::ostringstream oss;
    oss << "{\"gridSize\":" << config.gridSize
        << ",\"shuffleMoves\":" << config.shuffleMoves
        << ",\"difficulty\":\"" << Adapter::difficultyToString(config.difficulty) << "\""
        << ",\"enableHints\":" << (config.enableHints ? "true" : "false")
        << ",\"timeLimit\":" << config.timeLimit << "}";

    return oss.str();
}