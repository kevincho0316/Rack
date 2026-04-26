#include "Storage.h"
#include <fstream>

namespace fs = std::filesystem;

ServerStorage::ServerStorage(fs::path dir) : dataDir(std::move(dir)) {}

fs::path ServerStorage::projectDir(const std::string& p) const { return dataDir / p; }
fs::path ServerStorage::blobsDir(const std::string& p) const   { return dataDir / p / "blobs"; }
fs::path ServerStorage::platesDir(const std::string& p) const  { return dataDir / p / "plates"; }
fs::path ServerStorage::headFile(const std::string& p) const   { return dataDir / p / "HEAD"; }

void ServerStorage::init() {
    fs::create_directories(dataDir);
}

bool ServerStorage::projectExists(const std::string& p) const {
    return fs::exists(projectDir(p));
}

void ServerStorage::createProject(const std::string& p) {
    fs::create_directories(blobsDir(p));
    fs::create_directories(platesDir(p));
    std::ofstream{headFile(p)};
}

void ServerStorage::deleteProject(const std::string& p) {
    fs::remove_all(projectDir(p));
}

std::vector<std::string> ServerStorage::listProjects() const {
    std::vector<std::string> result;
    if (!fs::exists(dataDir)) return result;
    for (const auto& e : fs::directory_iterator(dataDir))
        if (e.is_directory()) result.push_back(e.path().filename().string());
    return result;
}

bool ServerStorage::blobExists(const std::string& p, const std::string& hash) const {
    return fs::exists(blobsDir(p) / hash);
}

std::string ServerStorage::readBlob(const std::string& p, const std::string& hash) const {
    std::ifstream f(blobsDir(p) / hash, std::ios::binary);
    if (!f.is_open()) return "";
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

void ServerStorage::writeBlob(const std::string& p, const std::string& hash, const std::string& data) {
    std::ofstream f(blobsDir(p) / hash, std::ios::binary);
    f << data;
}

bool ServerStorage::plateExists(const std::string& p, const std::string& id) const {
    return fs::exists(platesDir(p) / id);
}

std::string ServerStorage::readPlate(const std::string& p, const std::string& id) const {
    std::ifstream f(platesDir(p) / id);
    if (!f.is_open()) return "";
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

void ServerStorage::writePlate(const std::string& p, const std::string& id, const std::string& data) {
    std::ofstream f(platesDir(p) / id);
    f << data;
}

std::string ServerStorage::readHead(const std::string& p) const {
    std::ifstream f(headFile(p));
    if (!f.is_open()) return "";
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

void ServerStorage::writeHead(const std::string& p, const std::string& id) {
    std::ofstream f(headFile(p));
    f << id;
}

std::vector<std::string> ServerStorage::listPlateIds(const std::string& p) const {
    std::vector<std::string> result;
    if (!fs::exists(platesDir(p))) return result;
    for (const auto& e : fs::directory_iterator(platesDir(p)))
        if (e.is_regular_file()) result.push_back(e.path().filename().string());
    return result;
}
