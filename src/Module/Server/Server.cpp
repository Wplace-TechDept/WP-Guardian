#include "Server.h"
#include <iostream>
#include <asio.hpp>

Server::Server() {
    server.init_asio();
    server.set_reuse_addr(true);
}
Server::~Server() {
	stop();
}
void Server::start(const std::string& ip, uint16_t port) {
    if (running.exchange(true)) return;

    serverThread = std::thread(
        &Server::runServerThread, this, ip, port
    );
}

void Server::stop() {
    if (!running.exchange(false)) return;

    if (serverThread.joinable())
        serverThread.join();
}

void Server::runServerThread(const std::string& ip, uint16_t port)
{
    server.set_open_handler([this](websocketpp::connection_hdl hdl) {
        for (auto& fn : open_handlers) fn(hdl);
        });

    server.set_close_handler([this](websocketpp::connection_hdl hdl) {
        for (auto& fn : close_handlers) fn(hdl);
        });

    server.set_message_handler(
        [this](websocketpp::connection_hdl hdl, WSServer::message_ptr msg) {
            for (auto& fn : message_handlers) fn(hdl, msg);
        }
    );

    try {
        // ここで listen() する
        asio::ip::tcp::endpoint ep(asio::ip::make_address(ip), port);
        server.listen(ep);
        server.start_accept();
    }
    catch (const websocketpp::exception& e) {
        std::cerr << "サーバー起動失敗: " << e.what() << std::endl;
        return;
    }

    std::cout << "サーバー起動成功: " << ip << ":" << port << std::endl;

    while (running.load())
    {
        server.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server.stop_listening();
    server.stop();

}
void Server::add_open_handler(std::function<void(websocketpp::connection_hdl hdl)> handler) {
    if (running.load()) {
        throw std::logic_error("Cannot add handler after server start");
    }
    open_handlers.push_back(std::move(handler));
}
void Server::add_close_handler(std::function<void(websocketpp::connection_hdl hdl)> handler) {
    if (running.load()) {
        throw std::logic_error("Cannot add handler after server start");
    }
    close_handlers.push_back(std::move(handler));
}
void Server::add_message_handler(std::function<void(websocketpp::connection_hdl hdl, WSServer::message_ptr msg)> handler) {
    if (running.load()) {
        throw std::logic_error("Cannot add handler after server start");
    }
    message_handlers.push_back(std::move(handler));
}
void Server::send(websocketpp::connection_hdl hdl, const std::string& msg)
{
    if (!running.load()) return;

    asio::post(server.get_io_service(),
        [this, hdl, msg]()
        {
            websocketpp::lib::error_code ec;
            server.send(hdl, msg, websocketpp::frame::opcode::text, ec);

            if (ec) {
                std::cerr << "WebSocket send error: " << ec.message() << std::endl;
            }
        }
    );
}

