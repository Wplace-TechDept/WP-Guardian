#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;

template<typename T>
void assign_if_exists(const json& j, const char* key, T& dst)
{
    if (j.contains(key))
        dst = j[key].get<T>();
}