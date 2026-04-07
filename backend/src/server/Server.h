#pragma once

#include <string>
#include <memory>
#include <map>
#include "database/Database.h"
#include "adapter/Adapter.h"
#include "game/Game.h"

class Server {
public:
    Server(int port = 8080);
    ~Server();

    bool start();
    void stop();

private:
    int port_;
    bool running_;
    std::unique_ptr<Database> db_;
    std::unique_ptr<Adapter> adapter_;
    std::map<int, std::unique_ptr<Game>> games_;
    int nextSessionId_ = 1;

    // API обработчики
    std::string handleCreateGame(const std::string& body);
    std::string handleMove(const std::string& body);
    std::string handleStats();
    std::string handleConfig();
    std::string handleHint();
    std::string handleMastery();
    std::string handleStaticFile(const std::string& path);
};