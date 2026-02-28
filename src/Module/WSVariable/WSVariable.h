#pragma once
#ifndef _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#endif
#include <string>
#include <queue>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <set>
#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


using json = nlohmann::json;
using WSServer = websocketpp::server<websocketpp::config::asio>;

class WSVariable {
public:
    using Getter = std::function<json()>;
    using Setter = std::function<void(const json&)>;
    WSVariable();
    ~WSVariable();
    void update();

    void register_getter(const std::string& name, Getter getter);
	void register_setter(const std::string& name, Setter setter);

    void notify_change(const std::string& name);
    // WebSocketのメッセージハンドラ
    void on_message(websocketpp::connection_hdl hdl, WSServer::message_ptr msg);

    // WebSocketの接続・切断ハンドラ
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    std::function<void(websocketpp::connection_hdl, const std::string&)> send;
private:

    void broadcast(const std::string& name, const json& value);
private:

	std::mutex m_connections_mutex;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> m_connections;

    enum class VarReqType {
        Get,
        Set
    };
    struct VarRequest {
        VarReqType type;
        std::string name;   // 変数名 or グループ名
        json payload;       // SetVar用
        websocketpp::connection_hdl hdl;
    };
	std::mutex m_queue_mutex;
    std::queue<VarRequest> var_requests;

	std::unordered_map<std::string, Getter> getters;
	std::unordered_map<std::string, Setter> setters;
};