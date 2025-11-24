#ifndef IMWS_H
#define IMWS_H
#ifndef _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#endif

#include "imgui.h"
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <tuple>

// nlohmann::jsonのエイリアス
using json = nlohmann::json;

// WebSocketサーバーの型
using server = websocketpp::server<websocketpp::config::asio>;

// クライアントごとの描画データを一意に識別するためのキー
struct DrawDataKey
{
    websocketpp::connection_hdl hdl;
    std::string window_title;
    bool operator<(const DrawDataKey &other) const
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
// ウィンドウ
struct WindowState {
    bool visible = true;         // 表示/非表示
    bool marked_for_delete = false; // 削除フラグ

    std::vector<Widget> widgets; // 中に入っているUI部品たち
};

// ImGuiWebGuiクラスの定義
class ImWS
{
public:
    // 変数を登録するための関数ポインタ型
    using VarUpdater = std::function<json()>;

    // コンストラクタ
    ImWS(server &s);
    ~ImWS();

    // サーバーのハンドラ設定
    void set_server_handlers();

    // GUI描画の更新とWebSocket通信の処理
    void update();
    void drawWindowManager(bool& visible);

    // 変数をライブラリに登録
    template <typename T>
    void register_readonly_variable(const std::string& name, T& var);
	template <typename T>
	void register_writable_variable(const std::string& name, T& var);

private:
    // サーバーへの参照
    server &m_server;

    // スレッドセーフなアクセス用のミューテックス
    std::mutex m_mutex;

    std::map<DrawDataKey, WindowState> windows;

    // 同期する変数のマップ
    std::map<std::string, VarUpdater> m_readonly_vars;

    std::map<std::string, std::function<void(const json&)>> m_writable_vars;

    // WebSocketのメッセージハンドラ
    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

    // WebSocketの接続・切断ハンドラ
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);

    // クライアントへの安全なメッセージ送信
    void safe_send(websocketpp::connection_hdl hdl, const json &j);

};

// 変数登録関数のテンプレート実装
template <typename T>
void ImWS::register_readonly_variable(const std::string& name, T& var)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_readonly_vars[name] = [&var, name]() {
        json j;
        j["type"] = "variable_update";
        j["name"] = name;

        if constexpr (std::is_copy_constructible_v<T>) {
            j["value"] = var;  // コピー可能なら安全にコピー
        }
        else {
            j["value"] = nullptr;  // コピー不可能なら null
        }

        return j;
        };
}
template <typename T>
void ImWS::register_writable_variable(const std::string& name, T& var)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_writable_vars[name] = [&var](const json& j) {
        if (j.contains("value")) {
            try {
                var = j.at("value").get<T>();
            }
            catch (const std::exception& e) {
                std::cerr << "Error updating variable: " << e.what() << std::endl;
            }
        }
    };
}

Widget* find_widget_by_id_any(std::vector<Widget>& widgets, const std::string& id);

#endif // ImWS_H