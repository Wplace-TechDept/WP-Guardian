#include "Base64Codec.h"
#include "Base64-SIMD-C.h"
#include "base64.h"

Base64Codec::Base64Codec() {
	is_avx2_supported = b64_is_avx2_supported();
}

std::string Base64Codec::encode(const uint8_t* data, size_t size) {
    if (is_avx2_supported) {
        size_t enc_max = b64_encoded_size(size);
        std::string out(enc_max, '\0');

        size_t enc_actual = 0;
        base64_simd_encode(data, size, &enc_actual, out.data());
        out.resize(enc_actual);
        return out;
    }

    return Base64::Encode(reinterpret_cast<const unsigned char*>(data), size);
}
std::vector<uint8_t> Base64Codec::decode(const std::string& data) {
    if (is_avx2_supported) {
        size_t dec_max = b64_decoded_size(data.size());
        std::vector<uint8_t> out(dec_max);

        size_t dec_actual = 0;
        base64_simd_decode(data.data(), data.size(), &dec_actual, out.data());
        out.resize(dec_actual);
        return out;
    }

    return Base64::Decode(data);
}
