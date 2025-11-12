#include "ImWS.h"
#include <iostream>
#include <asio.hpp>

Widget* find_widget_by_id_any(std::vector<Widget>& widgets, const std::string& id) {
    for (auto& w : widgets) {
        bool match = std::visit([&](auto& widget) {
            return widget.id == id;
            }, w);

        if (match) return &w;
    }
    return nullptr;
}

// コンストラクタ
ImWS::ImWS(server &s) : m_server(s)
{
    // コンストラクタでの初期化が必要な場合はここに記述
}

// デストラクタ
ImWS::~ImWS()
{
    // デストラクタでのクリーンアップが必要な場合はここに記述
}

// サーバーのハンドラ設定
void ImWS::set_server_handlers()
{
    m_server.set_open_handler(std::bind(&ImWS::on_open, this, std::placeholders::_1));
    m_server.set_close_handler(std::bind(&ImWS::on_close, this, std::placeholders::_1));
    m_server.set_message_handler(std::bind(&ImWS::on_message, this, std::placeholders::_1, std::placeholders::_2));
}

// GUI描画の更新と通信の処理
void ImWS::update()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    for (auto it = windows.begin(); it != windows.end(); ) {
        if (it->second.marked_for_delete) it = windows.erase(it);
        else ++it;
    }
    for (auto& [key, window] : windows)
    {
        std::string win_title = key.window_title;
        if (window.visible) 
        {
            if (ImGui::Begin(win_title.c_str(), &window.visible))
            {
                for (auto& widget : window.widgets)
                {
                    if (std::holds_alternative<TextWidget>(widget))
                    {
                        auto& w = std::get<TextWidget>(widget);
                        ImGui::Text("%s", w.content.c_str());
                    }
                    else if (std::holds_alternative<ButtonWidget>(widget))
                    {
                        auto& w = std::get<ButtonWidget>(widget);
                        if (ImGui::Button(w.label.c_str()))
                        {
                            json j;
                            j["event"] = "button";
                            j["window"] = win_title;
                            j["id"] = w.id;
                            safe_send(key.hdl, j);
                        }
                    }
                    else if (std::holds_alternative<SliderWidget>(widget))
                    {
                        auto& w = std::get<SliderWidget>(widget);
                        if (ImGui::SliderFloat(w.label.c_str(), &w.value, w.min_val, w.max_val))
                        {
                            json j;
                            j["event"] = "slider";
                            j["window"] = win_title;
                            j["id"] = w.id;
                            j["value"] = w.value;
                            safe_send(key.hdl, j);
                        }
                    }
                    else if (std::holds_alternative<CheckboxWidget>(widget))
                    {
                        auto& w = std::get<CheckboxWidget>(widget);
                        if (ImGui::Checkbox(w.label.c_str(), &w.value))
                        {
                            json j;
                            j["event"] = "checkbox";
                            j["window"] = win_title;
                            j["id"] = w.id;
                            j["value"] = w.value;
                            safe_send(key.hdl, j);
                        }
                    }
                    else if (std::holds_alternative<InputTextWidget>(widget))
                    {
                        auto& w = std::get<InputTextWidget>(widget);
                        char buf[256];
                        strncpy(buf, w.value.c_str(), sizeof(buf));
                        buf[sizeof(buf) - 1] = '\0';
                        if (ImGui::InputText(w.label.c_str(), buf, sizeof(buf)))
                        {
                            w.value = std::string(buf);
                            json j;
                            j["event"] = "inputtext";
                            j["window"] = win_title;
                            j["id"] = w.id;
                            j["value"] = w.value;
                            safe_send(key.hdl, j);
                        }
                    }
                    else if (std::holds_alternative<ComboBoxWidget>(widget))
                    {
                        auto& w = std::get<ComboBoxWidget>(widget);
                        if (ImGui::BeginCombo(w.label.c_str(), w.items[w.value].c_str()))
                        {
                            for (size_t n = 0; n < w.items.size(); n++)
                            {
                                bool is_selected = (w.value == static_cast<int>(n));
                                if (ImGui::Selectable(w.items[n].c_str(), is_selected))
                                {
                                    w.value = static_cast<int>(n);
                                    json j;
                                    j["event"] = "combobox";
                                    j["window"] = win_title;
                                    j["id"] = w.id;
                                    j["value"] = w.value;
                                    safe_send(key.hdl, j);
                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    else if (std::holds_alternative<InputInt>(widget))
                    {
                        auto& w = std::get<InputInt>(widget);
                        if (ImGui::InputInt(w.label.c_str(), &w.value))
                        {
                            json j;
                            j["event"] = "inputint";
                            j["window"] = win_title;
                            j["id"] = w.id;
                            j["value"] = w.value;
                            safe_send(key.hdl, j);
                        }
                    }
                    else if (std::holds_alternative<InputFloat>(widget))
                    {
                        auto& w = std::get<InputFloat>(widget);
                        if (ImGui::InputFloat(w.label.c_str(), &w.value))
                        {
                            json j;
                            j["event"] = "inputfloat";
                            j["window"] = win_title;
                            j["id"] = w.id;
                            j["value"] = w.value;
                            safe_send(key.hdl, j);
                        }
                    }
                }
            }
            ImGui::End();
        }
    }

    lock.unlock();
}
void ImWS::drawWindowManager(bool& visible)
{
	std::lock_guard<std::mutex> lock(m_mutex);
    if (visible)
    {
        ImGui::Begin("Window Manager",&visible);
        for (auto& [key, window] : windows)
        {
            std::string win_title = key.window_title;
            // チェックボックスで表示/非表示を切り替え
            ImGui::Checkbox(win_title.c_str(), &window.visible);
        }

        ImGui::End();
    }
}
// WebSocketメッセージハンドラ
void ImWS::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    try
    {
        json j = json::parse(msg->get_payload());

        if (j.contains("createWindow"))
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            WindowState window;
            window.visible = true;
            window.marked_for_delete = false;
            std::string win_title = j.value("createWindow", "Default");

            if (j.contains("widgets"))
            {
                for (const auto &widget : j["widgets"])
                {
                    std::string type = widget.value("type", "");
                    std::string id = widget.value("id", "");
                    if (type == "text")
                    {
                        window.widgets.push_back(TextWidget{id, widget.value("content", "")});
                    }
                    else if (type == "button")
                    {
                        window.widgets.push_back(ButtonWidget{id, widget.value("label", "")});
                    }
                    else if (type == "slider")
                    {
                        window.widgets.push_back(SliderWidget{id, widget.value("label", ""), widget.value("value", 0.0f), widget.value("min", 0.0f), widget.value("max", 1.0f)});
                    }
                    else if (type == "checkbox")
                    {
                        window.widgets.push_back(CheckboxWidget{id, widget.value("value", false), widget.value("label", "")});
                    }
                    else if (type == "inputtext")
                    {
                        window.widgets.push_back(InputTextWidget{id, widget.value("value", ""), widget.value("label", "")});
                    }
                    else if (type == "combobox")
                    {
                        window.widgets.push_back(ComboBoxWidget{id, widget.value("value", 0), widget.value("items", std::vector<std::string>()), widget.value("label", "")});
                    }
                    else if (type == "inputint")
                    {
                        window.widgets.push_back(InputInt{id, widget.value("value", 0), widget.value("label", "")});
                    }
                    else if (type == "inputfloat")
                    {
                        window.widgets.push_back(InputFloat{ id, widget.value("value", 0.0f), widget.value("label", "")});
                    }
                }
            }
            

            windows[{hdl, win_title}] = window;
            std::cout << "Updated window: " << win_title << std::endl;
        }
        else if (j.contains("deleteWindow")) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string win_title = j.value("deleteWindow", "default");
            auto& window = windows[{ hdl, win_title }];
			window.marked_for_delete = true;
            
        }
        else if (j.contains("updateWindow")) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string win_title = j.value("updateWindow", "default");
            auto& window = windows[{ hdl,win_title }];

            if (j.contains("widgets"))
            {
                for (const auto& newWidget : j["widgets"])
                {
                    std::string id = newWidget.value("id", "");
                    auto* widget_ptr = find_widget_by_id_any(window.widgets, id); // ポインタを受け取る
                    if (widget_ptr) {
                        Widget& widget = *widget_ptr;  // 参照に変換
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
                }
            }
            
        }
        else if (j.contains("createWidget"))
        {
			std::lock_guard<std::mutex> lock(m_mutex);
            std::string win_title = j.value("createWidget", "default");
            auto& window = windows[{ hdl, win_title }];
            if (j.contains("widgets"))
            {
                for (const auto& widget : j["widgets"])
                {
                    std::string type = widget.value("type", "");
                    std::string id = widget.value("id", "");
                    if (type == "text")
                    {
                        window.widgets.push_back(TextWidget{id, widget.value("content", "")});
                    }
                    else if (type == "button")
                    {
                        window.widgets.push_back(ButtonWidget{id, widget.value("label", "")});
                    }
                    else if (type == "slider")
                    {
                        window.widgets.push_back(SliderWidget{id, widget.value("label", ""), widget.value("value", 0.0f), widget.value("min", 0.0f), widget.value("max", 1.0f)});
                    }
                    else if (type == "checkbox")
                    {
                        window.widgets.push_back(CheckboxWidget{id, widget.value("value", false), widget.value("label", "")});
                    }
                    else if (type == "inputtext")
                    {
                        window.widgets.push_back(InputTextWidget{id, widget.value("value", ""), widget.value("label", "")});
                    }
                    else if (type == "combobox")
                    {
                        window.widgets.push_back(ComboBoxWidget{id, widget.value("value", 0), widget.value("items", std::vector<std::string>()), widget.value("label", "")});
                    }
                    else if (type == "inputint")
                    {
                        window.widgets.push_back(InputInt{id, widget.value("value", 0), widget.value("label", "") });
                    }
                    else if (type == "inputfloat")
                    {
                        window.widgets.push_back(InputFloat{ id, widget.value("value", 0.0f), widget.value("label", "") });
                    }
                }
			}
        }
        else if (j.contains("deleteWidget"))
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string win_title = j.value("deleteWidget", "default");
            auto& window = windows[{ hdl, win_title }];
            if (j.contains("widgets"))
            {
                for (const auto& deleteWidget : j["widgets"])
                {
					std::string id = deleteWidget.value("id", "");
                    auto* widget_ptr = find_widget_by_id_any(window.widgets, id);
                    if (widget_ptr) 
                    {
                        auto it = std::find(window.widgets.begin(), window.widgets.end(), *widget_ptr);
                        if (it != window.widgets.end()) {
                            window.widgets.erase(it);
                        }
                    }

                }
            }
        }
        else if (j.contains("getVariable"))
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string name = j.value("getVariable", "");
            auto it = m_readonly_vars.find(name);
            if (it != m_readonly_vars.end())
            {
                json response = it->second();
                safe_send(hdl, response);
            }
            else
            {
                std::cerr << "Requested variable not found: " << name << std::endl;
            }
        }
        else if (j.contains("setVariable"))
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string name = j.value("setVariable", "");
			auto it = m_writable_vars.find(name);
            if (it != m_writable_vars.end())
            {
                json j_value = { {"value",j["value"]}};
                it->second(j_value);
            }
            else
            {
                std::cerr << "Requested variable not found or not writable: " << name << std::endl;
			}
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
}


// 接続ハンドラ
void ImWS::on_open(websocketpp::connection_hdl hdl)
{
    // 接続時に何か処理が必要な場合はここに記述
}

// 切断ハンドラ
void ImWS::on_close(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto closing = hdl.lock();
    for (auto it = windows.begin(); it != windows.end(); )
    {
        auto current = it->first.hdl.lock();
        if (closing && current && closing == current)
        {
            it = windows.erase(it);
        }
        else
        {
            ++it;
        }
    }
}



// 安全なメッセージ送信
void ImWS::safe_send(websocketpp::connection_hdl hdl, const json &j)
{
    auto msg = j.dump();
    asio::post(m_server.get_io_service(), [hdl, msg, this]()
    {
        try {
            m_server.send(hdl, msg, websocketpp::frame::opcode::text);
        } catch (const websocketpp::exception& e) {
            std::cerr << "WebSocket send error: " << e.what() << std::endl;
        } 
     });
}