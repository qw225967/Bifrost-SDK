#include "websocket_pub.hpp"
#include "utils/base64.hpp"
#include <stdint.h>
#include <string>
#include <string.h>
#ifdef ENABLE_OPEN_SSL
#include <openssl/sha.h>
#else
#include "utils/sha1.hpp"
#endif

namespace cpp_streamer
{
std::string  GenWebSocketHashcode(const std::string& key) {
    std::string sec_key = key;
	sec_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#ifdef ENABLE_OPEN_SSL
	unsigned char hash[20];
    SHA_CTX sha1;

    SHA1_Init(&sha1);
    SHA1_Update(&sha1, sec_key.data(), sec_key.size());
    SHA1_Final(hash, &sha1);
	
	return Base64Encode(hash, sizeof(hash));
#else
    char hex[SHA1_HEX_SIZE];
    char base64[SHA1_BASE64_SIZE];

    sha1(sec_key.c_str())
        .finalize()
        .print_hex(hex)
        .print_base64(base64);
    return std::string(base64, strlen(base64));
#endif
}

}