#pragma once
#include <string>
#include <cstdint>
#include <vector>

class Base64Codec {
public:
	Base64Codec();
	std::string encode(const uint8_t* data, size_t size);
	std::vector<uint8_t> decode(const std::string& data);
private:
	bool is_avx2_supported = false;
};