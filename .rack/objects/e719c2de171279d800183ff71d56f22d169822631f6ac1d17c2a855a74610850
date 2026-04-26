#include "ObjectStore.h"
#include <fstream>

ObjectStore::ObjectStore(std::filesystem::path objDir, std::filesystem::path initDir)
    : objectsDir(std::move(objDir)), initFileDir(std::move(initDir)) {}

bool ObjectStore::exists(const std::string& hash) const {
    return std::filesystem::exists(objectsDir / hash);
}

std::string ObjectStore::read(const std::string& hash) const {
    std::ifstream file(objectsDir / hash, std::ios::binary);
    if (!file.is_open()) return "";
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::string ObjectStore::readInit() const {
    std::ifstream file(initFileDir, std::ios::binary);
    if (!file.is_open()) return "";
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void ObjectStore::write(const std::string& hash, const std::string& data) {
    std::ofstream file(objectsDir / hash, std::ios::binary);
    file << data;
    newFile.push_back(hash);
}

void ObjectStore::writeInit(const std::string& hash) {
    std::ofstream file(initFileDir, std::ios::binary);
    file << hash;
}
