#include "WSVariable.h"

WSVariable::WSVariable()
{
    // コンストラクタでの初期化が必要な場合はここに記述
}
WSVariable::~WSVariable()
{
    // デストラクタでのクリーンアップが必要な場合はここに記述
}


void WSVariable::register_getter(const std::string& name, Getter getter)
{
    getters[name] = getter;
}

void WSVariable::register_setter(const std::string& name, Setter setter)
{
    setters[name] = setter;
}


void WSVariable::update()
{
    std::queue<VarRequest> local;
    {
        std::lock_guard lock(m_queue_mutex);
        std::swap(local, var_requests);
    }

    while (!local.empty()) {
        VarRequest& req = local.front();

        if (req.type == VarReqType::Get) {
            // Getter呼び出し
            auto it = getters.find(req.name);
            if (it != getters.end()) {
                try {
                    json value = it->second();
                    json response;
                    response["target"] = "var";
                    response["cmd"] = "get_response";
                    response["name"] = req.name;
                    response["value"] = value;
                    response["success"] = true;
                    send(req.hdl, response.dump());
                }
                catch (const std::exception& e) {
                    json error_response;
                    error_response["target"] = "var";
                    error_response["cmd"] = "error";
                    error_response["name"] = req.name;
                    error_response["message"] = std::string("Getter error: ") + e.what();
                    error_response["success"] = false;
                    send(req.hdl, error_response.dump());
                }
            }
            else {
                // Getterが見つからない
                json error_response;
                error_response["target"] = "var";
                error_response["cmd"] = "error";
                error_response["name"] = req.name;
                error_response["message"] = "Variable not found";
                error_response["success"] = false;
                send(req.hdl, error_response.dump());
            }
        }
        else if (req.type == VarReqType::Set) {
            // Setter呼び出し
            auto it = setters.find(req.name);
            if (it != setters.end()) {
                try {
                    it->second(req.payload);
                    json response;
                    response["target"] = "var";
                    response["cmd"] = "set_response";
                    response["name"] = req.name;
                    response["success"] = true;
                    send(req.hdl, response.dump());
                }
                catch (const std::exception& e) {
                    json error_response;
                    error_response["target"] = "var";
                    error_response["cmd"] = "error";
                    error_response["name"] = req.name;
                    error_response["message"] = std::string("Setter error: ") + e.what();
                    error_response["success"] = false;
                    send(req.hdl, error_response.dump());
                }
            }
            else {
                // Setterが見つからない
                json error_response;
                error_response["target"] = "var";
                error_response["cmd"] = "error";
                error_response["name"] = req.name;
                error_response["message"] = "Variable not found or read-only";
                error_response["success"] = false;
                send(req.hdl, error_response.dump());
            }
        }

        local.pop();
    }
}

void WSVariable::on_message(websocketpp::connection_hdl hdl, WSServer::message_ptr msg)
{
    json j;
    try {
        j = json::parse(msg->get_payload());
    }
    catch (...) {
        return; // JSONとして無効 → 自分の担当じゃない
    }

    if (!j.contains("target") || j["target"] != "var") {
        return;
    }
    try {
        // 必須フィールドのチェック
        if (!j.contains("cmd") || !j.contains("name")) {
            json error_response;
            error_response["target"] = "var";
            error_response["cmd"] = "error";
            error_response["message"] = "Missing required fields: cmd and name";
            error_response["success"] = false;
            send(hdl, error_response.dump());
            return;
        }

        VarRequest req;
        req.hdl = hdl;
        req.name = j["name"];

        if (j["cmd"] == "get") {
            req.type = VarReqType::Get;
        }
        else if (j["cmd"] == "set") {
            if (!j.contains("value")) {
                json error_response;
                error_response["target"] = "var";
                error_response["cmd"] = "error";
                error_response["name"] = req.name;
                error_response["message"] = "Missing 'value' field for set command";
                error_response["success"] = false;
                send(hdl, error_response.dump());
                return;
            }
            req.type = VarReqType::Set;
            req.payload = j["value"];
        }
        else {
            // 不明なコマンド
            json error_response;
            error_response["target"] = "var";
            error_response["cmd"] = "error";
            error_response["name"] = req.name;
            error_response["message"] = "Unknown command: " + j["cmd"].get<std::string>();
            error_response["success"] = false;
            send(hdl, error_response.dump());
            return;
        }

        {
            std::lock_guard lock(m_queue_mutex);
            var_requests.push(std::move(req));
        }
    }
    catch (const json::parse_error& e) {
        json error_response;
        error_response["target"] = "var";
        error_response["cmd"] = "error";
        error_response["message"] = std::string("JSON parse error: ") + e.what();
        error_response["success"] = false;
        send(hdl, error_response.dump());
    }
    catch (const std::exception& e) {
        json error_response;
        error_response["target"] = "var";
        error_response["cmd"] = "error";
        error_response["message"] = std::string("Error: ") + e.what();
        error_response["success"] = false;
        send(hdl, error_response.dump());
    }
}

void WSVariable::on_open(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    m_connections.insert(hdl);
}
void WSVariable::on_close(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    m_connections.erase(hdl);
}

void WSVariable::broadcast(const std::string& name, const json& value)
{
    json notification;
    notification["target"] = "var";
    notification["cmd"] = "notify";
    notification["name"] = name;
    notification["value"] = value;

    std::vector<websocketpp::connection_hdl> connections_copy;
    {
        std::lock_guard<std::mutex> lock(m_connections_mutex);
        connections_copy.assign(m_connections.begin(), m_connections.end());
    }

    for (auto& hdl : connections_copy) {
        send(hdl, notification.dump());
    }
}
void WSVariable::notify_change(const std::string& name)
{
    auto it = getters.find(name);
    if (it == getters.end()) {
        return;  // Getterが登録されていない
    }

    try {
        json value = it->second();
        broadcast(name, value);
    }
    catch (const std::exception& e) {
        std::cerr << "Error in notify_change: " << e.what() << std::endl;
    }
}