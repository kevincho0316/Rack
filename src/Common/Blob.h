#pragma once
#include <string>
#include "hash.h"

struct Blob {
    std::string content;
    std::string generate_blob_hash() const { return GetHash(content); }
};
