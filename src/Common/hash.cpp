#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <openssl/evp.h>
#include <sstream>
#include "hash.h"

std::string GetHash(const std::string& str) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;

    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, str.c_str(), str.size());
    EVP_DigestFinal_ex(context, hash, &length);

    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for(unsigned int i = 0; i < length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
};

