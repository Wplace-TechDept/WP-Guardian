#ifndef BASE64_SIMD_C_H
#define BASE64_SIMD_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#ifdef BUILDING_DLL
#define B64_API __declspec(dllexport)
#elif defined(USE_DLL)
#define B64_API __declspec(dllimport)
#else
#define B64_API 
#endif
#else
#define B64_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif
    B64_API int b64_is_avx2_supported();    //Supported: 1, Not supported: 0

    B64_API size_t b64_encoded_size(size_t input_len);
    B64_API size_t b64_decoded_size(size_t input_len);
    B64_API void base64_simd_encode(const uint8_t* src, size_t len, size_t* out_len, char* out);
    B64_API void base64_simd_decode(const char* src, size_t len, size_t* out_len, uint8_t* out);

#ifdef __cplusplus
}
#endif

#endif