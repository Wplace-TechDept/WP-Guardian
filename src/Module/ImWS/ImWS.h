#ifndef IMWS_H
#define IMWS_H
#ifndef _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#endif

#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <mutex>

// nlohmann::jsonのエイリアス
using json = nlohmann::json;

// WebSocketサーバーの型
using WSServer = websocketpp::server<websocketpp::config::asio>;

// クライアントごとの描画データを一意に識別するためのキー
struct DrawDataKey
{
    websocketpp::connection_hdl hdl;
    std::string window_title;

    bool operator<(const DrawDataKey& other) const
    {
        if (hdl.owner_before(other.hdl))
            return true;
        if (other.hdl.owner_before(hdl))
            return false;
        return window_title < other.window_title;
    }
};

// 各UI部品の型
struct TextWidget {
    std::string id;
    std::string content;

    bool operator==(const TextWidget& other) const {
        return id == other.id;
    }
};

struct ButtonWidget {
    std::string id;
    std::string label;

    bool operator==(const ButtonWidget& other) const {
        return id == other.id;
    }
};

struct SliderWidget {
    std::string id;
    std::string label;
    float value;
    float min_val;
    float max_val;

    bool operator==(const SliderWidget& other) const {
        return id == other.id;
    }
};

struct CheckboxWidget {
    std::string id;
    bool value;
    std::string label;

    bool operator==(const CheckboxWidget& other) const {
        return id == other.id;
    }
};

struct InputTextWidget {
    std::string id;
    std::string value;
    std::string label;

    bool operator==(const InputTextWidget& other) const {
        return id == other.id;
    }
};

struct ComboBoxWidget {
    std::string id;
    int value;
    std::vector<std::string> items;
    std::string label;

    bool operator==(const ComboBoxWidget& other) const {
        return id == other.id;
    }
};

struct InputInt {
    std::string id;
    int value;
    std::string label;

    bool operator==(const InputInt& other) const {
        return id == other.id;
    }
};

struct InputFloat {
    std::string id;
    float value;
    std::string label;

    bool operator==(const InputFloat& other) const {
        return id == other.id;
    }
};

// UI要素の総称型
using Widget = std::variant<
    TextWidget,
    ButtonWidget,
    SliderWidget,
    CheckboxWidget,
    InputTextWidget,
    ComboBoxWidget,
    InputInt,
    InputFloat
>;

// ウィンドウの状態
struct WindowState {
    bool visible = true;
    bool marked_for_delete = false;
    std::vector<Widget> widgets;
};

// ImGuiとWebSocketを統合するクラス
class ImWS
{
public:
    ImWS();
    ~ImWS();

    // GUI描画の更新
    void update();

    void on_message(websocketpp::connection_hdl hdl, WSServer::message_ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    std::function<void(websocketpp::connection_hdl, const std::string&)> send;
    // ウィンドウマネージャーの描画
    void drawWindowManager(bool& visible);

private:
    std::mutex m_mutex;
    std::map<DrawDataKey, WindowState> windows;

    // WebSocketイベントハンドラ



    // コマンド処理 (private helper functions)
    void handle_create_window(websocketpp::connection_hdl hdl, const json& payload);
    void handle_delete_window(websocketpp::connection_hdl hdl, const json& payload);
    void handle_update_window(websocketpp::connection_hdl hdl, const json& payload);
    void handle_create_widget(websocketpp::connection_hdl hdl, const json& payload);
    void handle_delete_widget(websocketpp::connection_hdl hdl, const json& payload);

    // ウィジェット処理ヘルパー
    void render_widget(Widget& widget, const std::string& win_title, websocketpp::connection_hdl hdl);
    void update_widget_from_json(Widget& widget, const json& newWidget);

    // ウィンドウ検索
    WindowState* find_window(websocketpp::connection_hdl hdl, const std::string& title);
};

#endif // IMWS_H