#pragma once
#include <string>
#include <vector>

namespace Base64 {
    std::string Encode(const unsigned char* data, size_t len);
    std::string Encode(const std::vector<unsigned char>& data);
    std::string Encode(const std::string& data);

    std::vector<unsigned char> Decode(const std::string& encoded);
}
