#pragma once
#ifndef _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#endif
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

using WSServer = websocketpp::server<websocketpp::config::asio>;

class Server {
public:
    Server();
    ~Server();

    void start(const std::string& ip, uint16_t port);
    void stop();

    void add_open_handler(std::function<void(websocketpp::connection_hdl hdl) > );
	void add_close_handler(std::function<void(websocketpp::connection_hdl hdl) >);
	void add_message_handler(std::function<void(websocketpp::connection_hdl hdl, WSServer::message_ptr msg)>);
    void send(websocketpp::connection_hdl hdl, const std::string& msg);

private:

    void runServerThread(const std::string& ip, uint16_t port);

	std::vector < std::function<void(websocketpp::connection_hdl) >> open_handlers;
	std::vector < std::function<void(websocketpp::connection_hdl) >> close_handlers;
	std::vector < std::function<void(websocketpp::connection_hdl, WSServer::message_ptr)>> message_handlers;

    WSServer server;
    std::thread serverThread;
    std::atomic<bool> running{ false };
};
