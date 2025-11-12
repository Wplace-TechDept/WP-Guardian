#pragma once
#include <nlohmann/json.hpp>

// ---------------------
// DiffPattern
// ---------------------
enum class DiffPatternType { AbsDiff, ColorMask, OverlayTemplate };

struct DiffPattern
{
    DiffPatternType type;

    // デフォルトコンストラクタも追加
    DiffPattern() : type(DiffPatternType::AbsDiff) {}
    DiffPattern(DiffPatternType t) : type(t) {}
};

// JSON 変換関数
inline void to_json(nlohmann::json& j, const DiffPattern& p) {
    j = nlohmann::json{
        {"type", static_cast<int>(p.type)}
    };
}

inline void from_json(const nlohmann::json& j, DiffPattern& p) {
    p.type = static_cast<DiffPatternType>(j.at("type").get<int>());
}

// ---------------------
// ImVec4
// ---------------------
#include <imgui.h>

inline void to_json(nlohmann::json& j, const ImVec4& c) {
    j = { c.x, c.y, c.z, c.w };
}

inline void from_json(const nlohmann::json& j, ImVec4& c) {
    c.x = j.at(0).get<float>();
    c.y = j.at(1).get<float>();
    c.z = j.at(2).get<float>();
    c.w = j.at(3).get<float>();
}
