#include "ImWS.h"
#include <iostream>
#include <asio.hpp>

namespace {
    constexpr size_t INPUT_TEXT_BUFFER_SIZE = 256;
    constexpr const char* DEFAULT_WINDOW_TITLE = "Default";
}

// ヘルパー関数: ウィジェット検索
Widget* find_widget_by_id_any(std::vector<Widget>& widgets, const std::string& id) {
    for (auto& w : widgets) {
        bool match = std::visit([&](auto& widget) {
            return widget.id == id;
            }, w);
        if (match) return &w;
    }
    return nullptr;
}

// ヘルパー関数: JSONからウィジェット作成
Widget create_widget_from_json(const json& widget) {
    std::string type = widget.value("type", "");
    std::string id = widget.value("id", "");

    if (type == "text") {
        return TextWidget{ id, widget.value("content", "") };
    }
    else if (type == "button") {
        return ButtonWidget{ id, widget.value("label", "") };
    }
    else if (type == "slider") {
        return SliderWidget{
            id,
            widget.value("label", ""),
            widget.value("value", 0.0f),
            widget.value("min", 0.0f),
            widget.value("max", 1.0f)
        };
    }
    else if (type == "checkbox") {
        return CheckboxWidget{
            id,
            widget.value("value", false),
            widget.value("label", "")
        };
    }
    else if (type == "inputtext") {
        return InputTextWidget{
            id,
            widget.value("value", ""),
            widget.value("label", "")
        };
    }
    else if (type == "combobox") {
        return ComboBoxWidget{
            id,
            widget.value("value", 0),
            widget.value("items", std::vector<std::string>()),
            widget.value("label", "")
        };
    }
    else if (type == "inputint") {
        return InputInt{
            id,
            widget.value("value", 0),
            widget.value("label", "")
        };
    }
    else if (type == "inputfloat") {
        return InputFloat{
            id,
            widget.value("value", 0.0f),
            widget.value("label", "")
        };
    }

    // デフォルト: 空のテキストウィジェット
    return TextWidget{ id, "" };
}

ImWS::ImWS() {}

ImWS::~ImWS() {}



// ウィジェット描画を個別関数に分離
void ImWS::render_widget(Widget& widget, const std::string& win_title, websocketpp::connection_hdl hdl) {
    if (std::holds_alternative<TextWidget>(widget)) {
        auto& w = std::get<TextWidget>(widget);
        ImGui::Text("%s", w.content.c_str());
    }
    else if (std::holds_alternative<ButtonWidget>(widget)) {
        auto& w = std::get<ButtonWidget>(widget);
        if (ImGui::Button(w.label.c_str())) {
            json j;
            j["target"] = "ui";
            j["event"] = "button";
            j["window"] = win_title;
            j["id"] = w.id;
            send(hdl, j.dump());
        }
    }
    else if (std::holds_alternative<SliderWidget>(widget)) {
        auto& w = std::get<SliderWidget>(widget);
        if (ImGui::SliderFloat(w.label.c_str(), &w.value, w.min_val, w.max_val)) {
            json j;
            j["target"] = "ui";
            j["event"] = "slider";
            j["window"] = win_title;
            j["id"] = w.id;
            j["value"] = w.value;
            send(hdl, j.dump());
        }
    }
    else if (std::holds_alternative<CheckboxWidget>(widget)) {
        auto& w = std::get<CheckboxWidget>(widget);
        if (ImGui::Checkbox(w.label.c_str(), &w.value)) {
            json j;
            j["target"] = "ui";
            j["event"] = "checkbox";
            j["window"] = win_title;
            j["id"] = w.id;
            j["value"] = w.value;
            send(hdl, j.dump());
        }
    }
    else if (std::holds_alternative<InputTextWidget>(widget)) {
        auto& w = std::get<InputTextWidget>(widget);
        char buf[INPUT_TEXT_BUFFER_SIZE];
        strncpy(buf, w.value.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText(w.label.c_str(), buf, sizeof(buf))) {
            w.value = std::string(buf);
            json j;
            j["target"] = "ui";
            j["event"] = "inputtext";
            j["window"] = win_title;
            j["id"] = w.id;
            j["value"] = w.value;
            send(hdl, j.dump());
        }
    }
    else if (std::holds_alternative<ComboBoxWidget>(widget)) {
        auto& w = std::get<ComboBoxWidget>(widget);
        if (ImGui::BeginCombo(w.label.c_str(), w.items[w.value].c_str())) {
            for (size_t n = 0; n < w.items.size(); n++) {
                bool is_selected = (w.value == static_cast<int>(n));
                if (ImGui::Selectable(w.items[n].c_str(), is_selected)) {
                    w.value = static_cast<int>(n);
                    json j;
                    j["target"] = "ui";
                    j["event"] = "combobox";
                    j["window"] = win_title;
                    j["id"] = w.id;
                    j["value"] = w.value;
                    send(hdl, j.dump());
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    else if (std::holds_alternative<InputInt>(widget)) {
        auto& w = std::get<InputInt>(widget);
        if (ImGui::InputInt(w.label.c_str(), &w.value)) {
            json j;
            j["target"] = "ui";
            j["event"] = "inputint";
            j["window"] = win_title;
            j["id"] = w.id;
            j["value"] = w.value;
            send(hdl, j.dump());
        }
    }
    else if (std::holds_alternative<InputFloat>(widget)) {
        auto& w = std::get<InputFloat>(widget);
        if (ImGui::InputFloat(w.label.c_str(), &w.value)) {
            json j;
            j["target"] = "ui";
            j["event"] = "inputfloat";
            j["window"] = win_title;
            j["id"] = w.id;
            j["value"] = w.value;
            send(hdl, j.dump());
        }
    }
}

void ImWS::update() {
    std::unique_lock<std::mutex> lock(m_mutex);

    // 削除マークされたウィンドウを削除
    for (auto it = windows.begin(); it != windows.end(); ) {
        if (it->second.marked_for_delete) {
            it = windows.erase(it);
        }
        else {
            ++it;
        }
    }

    // 各ウィンドウを描画
    for (auto& [key, window] : windows) {
        if (!window.visible) continue;

        std::string win_title = key.window_title;
        if (ImGui::Begin(win_title.c_str(), &window.visible)) {
            for (auto& widget : window.widgets) {
                render_widget(widget, win_title, key.hdl);
            }
        }
        ImGui::End();
    }

    lock.unlock();
}

void ImWS::drawWindowManager(bool& visible) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!visible) return;

    if (ImGui::Begin("Window Manager", &visible)) {
        for (auto& [key, window] : windows) {
            std::string win_title = key.window_title;
            ImGui::Checkbox(win_title.c_str(), &window.visible);
        }
    }
    ImGui::End();
}

// ウィンドウ検索ヘルパー
WindowState* ImWS::find_window(websocketpp::connection_hdl hdl, const std::string& title) {
    auto it = windows.find({ hdl, title });
    if (it != windows.end()) {
        return &it->second;
    }
    return nullptr;
}

// ウィジェット更新処理を分離
void ImWS::update_widget_from_json(Widget& widget, const json& newWidget) {
    if (std::holds_alternative<TextWidget>(widget)) {
        auto& w = std::get<TextWidget>(widget);
        w.content = newWidget.value("content", w.content);
    }
    else if (std::holds_alternative<ButtonWidget>(widget)) {
        auto& w = std::get<ButtonWidget>(widget);
        w.label = newWidget.value("label", w.label);
    }
    else if (std::holds_alternative<SliderWidget>(widget)) {
        auto& w = std::get<SliderWidget>(widget);
        w.value = newWidget.value("value", w.value);
        w.min_val = newWidget.value("min", w.min_val);
        w.max_val = newWidget.value("max", w.max_val);
    }
    else if (std::holds_alternative<CheckboxWidget>(widget)) {
        auto& w = std::get<CheckboxWidget>(widget);
        w.value = newWidget.value("value", w.value);
        w.label = newWidget.value("label", w.label);
    }
    else if (std::holds_alternative<InputTextWidget>(widget)) {
        auto& w = std::get<InputTextWidget>(widget);
        w.value = newWidget.value("value", w.value);
        w.label = newWidget.value("label", w.label);
    }
    else if (std::holds_alternative<ComboBoxWidget>(widget)) {
        auto& w = std::get<ComboBoxWidget>(widget);
        w.value = newWidget.value("value", w.value);
        w.items = newWidget.value("items", w.items);
        w.label = newWidget.value("label", w.label);
    }
    else if (std::holds_alternative<InputInt>(widget)) {
        auto& w = std::get<InputInt>(widget);
        w.value = newWidget.value("value", w.value);
    }
    else if (std::holds_alternative<InputFloat>(widget)) {
        auto& w = std::get<InputFloat>(widget);
        w.value = newWidget.value("value", w.value);
    }
}

// コマンド処理を個別関数に分離
void ImWS::handle_create_window(websocketpp::connection_hdl hdl, const json& payload) {
    WindowState window;
    window.visible = true;
    window.marked_for_delete = false;
    std::string win_title = payload.value("title", DEFAULT_WINDOW_TITLE);

    if (payload.contains("widgets")) {
        for (const auto& widget_json : payload["widgets"]) {
            window.widgets.push_back(create_widget_from_json(widget_json));
        }
    }

    windows[{hdl, win_title}] = std::move(window);
    std::cout << "Created window: " << win_title << std::endl;
}

void ImWS::handle_delete_window(websocketpp::connection_hdl hdl, const json& payload) {
    std::string win_title = payload.value("title", DEFAULT_WINDOW_TITLE);
    auto* window = find_window(hdl, win_title);
    if (window) {
        window->marked_for_delete = true;
        std::cout << "Marked window for deletion: " << win_title << std::endl;
    }
}

void ImWS::handle_update_window(websocketpp::connection_hdl hdl, const json& payload) {
    std::string win_title = payload.value("title", DEFAULT_WINDOW_TITLE);
    auto* window = find_window(hdl, win_title);
    if (!window) {
        std::cerr << "Window not found: " << win_title << std::endl;
        return;
    }

    if (payload.contains("widgets")) {
        for (const auto& newWidget : payload["widgets"]) {
            std::string id = newWidget.value("id", "");
            auto* widget_ptr = find_widget_by_id_any(window->widgets, id);
            if (widget_ptr) {
                update_widget_from_json(*widget_ptr, newWidget);
            }
        }
    }
}

void ImWS::handle_create_widget(websocketpp::connection_hdl hdl, const json& payload) {
    std::string win_title = payload.value("title", DEFAULT_WINDOW_TITLE);
    auto* window = find_window(hdl, win_title);
    if (!window) {
        std::cerr << "Window not found: " << win_title << std::endl;
        return;
    }

    if (payload.contains("widgets")) {
        for (const auto& widget_json : payload["widgets"]) {
            window->widgets.push_back(create_widget_from_json(widget_json));
        }
    }
}

void ImWS::handle_delete_widget(websocketpp::connection_hdl hdl, const json& payload) {
    std::string win_title = payload.value("title", DEFAULT_WINDOW_TITLE);
    auto* window = find_window(hdl, win_title);
    if (!window) {
        std::cerr << "Window not found: " << win_title << std::endl;
        return;
    }

    if (payload.contains("widgets")) {
        for (const auto& delWidget : payload["widgets"]) {
            std::string id = delWidget.value("id", "");
            auto* widget_ptr = find_widget_by_id_any(window->widgets, id);
            if (widget_ptr) {
                auto it = std::find(window->widgets.begin(), window->widgets.end(), *widget_ptr);
                if (it != window->widgets.end()) {
                    window->widgets.erase(it);
                }
            }
        }
    }
}

void ImWS::on_message(websocketpp::connection_hdl hdl, WSServer::message_ptr msg) {
    json j;
    try {
        j = json::parse(msg->get_payload());
    }
    catch (...) {
        return; // JSONとして無効 → 自分の担当じゃない
    }

    if (!j.contains("target") || j["target"] != "ui") {
        return;
    }
    try {
        std::string cmd = j.value("command", "");
        json payload = j.value("payload", j);

        std::lock_guard<std::mutex> lock(m_mutex);

        if (cmd == "createWindow") {
            handle_create_window(hdl, payload);
        }
        else if (cmd == "deleteWindow") {
            handle_delete_window(hdl, payload);
        }
        else if (cmd == "updateWindow") {
            handle_update_window(hdl, payload);
        }
        else if (cmd == "createWidget") {
            handle_create_widget(hdl, payload);
        }
        else if (cmd == "deleteWidget") {
            handle_delete_widget(hdl, payload);
        }
        else {
            std::cerr << "Unknown command: " << cmd << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
}

void ImWS::on_open(websocketpp::connection_hdl hdl) {
    auto con = hdl.lock();
    if (con) {
        std::cout << "Connection opened" << std::endl;
    }
}

void ImWS::on_close(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto closing = hdl.lock();
    if (!closing) return;

    for (auto it = windows.begin(); it != windows.end(); ) {
        auto current = it->first.hdl.lock();
        if (current && closing == current) {
            it = windows.erase(it);
        }
        else {
            ++it;
        }
    }

    std::cout << "Connection closed" << std::endl;
}
