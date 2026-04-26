#include "Rack.h"
#include "Paths.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

static std::string extractField(const std::string& data, const std::string& key) {
    size_t pos = data.find(key);
    if (pos == std::string::npos) return "";
    size_t start = pos + key.size();
    size_t end   = data.find('\n', start);
    return (end == std::string::npos) ? data.substr(start) : data.substr(start, end - start);
}

void Rack::loadConfig() {
    std::ifstream f(RackPaths::configFile);
    if (!f.is_open()) return;
    try {
        auto j = json::parse(f);
        if (j.contains("domain"))  api.domain  = j["domain"];
        if (j.contains("project")) api.project = j["project"];
    } catch (...) {}
}

void Rack::saveConfig() {
    std::ofstream f(RackPaths::configFile);
    json j = {{"domain", api.domain}, {"project", api.project}};
    f << j.dump(2);
}

Rack::Rack()
    : store(RackPaths::objects, RackPaths::initFile),
      builder(store),
      reader(store),
      api(store)
{
    if (!fs::exists(RackPaths::objects))  fs::create_directories(RackPaths::objects);
    if (!fs::exists(RackPaths::initFile)) std::ofstream{RackPaths::initFile};
    loadConfig();
}

std::string Rack::commit() {
    return builder.createPlateFile();
}

std::string Rack::readFile(const std::string& hash) {
    if (hash == "init") return store.readInit();
    return store.read(hash);
}

std::vector<std::string> Rack::ls() {
    return reader.ExtractHash(reader.getInitPlate(), "[Hash]");
}

void Rack::reconstruct(const fs::path& dest) {
    reader.reconstruct(reader.getInitPlate(), dest);
}

void Rack::setDomain(const std::string& domain) {
    api.domain = domain;
    saveConfig();
}

bool Rack::isServerOn() {
    return api.isServerOn();
}

void Rack::push() {
    if (api.project.empty()) {
        std::cout << "[WARN] No project set. Run: rack init <project>\n";
        return;
    }

    std::string plateHash = store.readInit();
    if (plateHash.empty()) { std::cout << "Nothing to push — no commits\n"; return; }

    std::string plateData  = store.read(plateHash);
    std::string parentHash = extractField(plateData, "[Parent]");
    std::string treeHash   = extractField(plateData, "[Tree]");

    auto flatTree = reader.flattenTree(treeHash);
    if (flatTree.empty()) { std::cout << "Empty tree — nothing to push\n"; return; }

    std::vector<std::string> allHashes;
    for (const auto& [path, hash] : flatTree) allHashes.push_back(hash);

    std::vector<std::string> missing = api.checkBlobs(allHashes);
    std::cout << "Blobs: " << allHashes.size() << " total, "
              << missing.size() << " to upload\n";

    for (const auto& hash : missing) {
        std::string data = store.read(hash);
        std::string serverHash = api.uploadBlob(data);
        if (serverHash.empty()) { std::cout << "Upload failed for " << hash << "\n"; return; }
    }

    std::string plateId = api.createPlate(parentHash, flatTree, plateHash.substr(0, 8));
    if (!plateId.empty()) std::cout << "Pushed plate: " << plateId << "\n";
}

void Rack::pull() {
    if (api.project.empty()) {
        std::cout << "[WARN] No project set. Run: rack init <project>\n";
        return;
    }

    auto tree = api.fetchLatestTree();
    if (tree.empty()) { std::cout << "Nothing to pull\n"; return; }

    std::cout << "Pulling " << tree.size() << " files\n";
    for (const auto& [path, hash] : tree) {
        if (path.rfind(".rack", 0) == 0) continue;
        std::string data = api.downloadBlob(hash);
        if (data.empty()) continue;

        fs::path fpath(path);
        if (fpath.has_parent_path()) fs::create_directories(fpath.parent_path());
        std::ofstream f(fpath, std::ios::binary);
        f << data;
    }
    std::cout << "Pull complete\n";
}

bool Rack::initProject(const std::string& name) {
    bool ok = api.initProject(name);
    if (ok) {
        api.project = name;
        saveConfig();
        std::cout << "Project '" << name << "' active\n";
    }
    return ok;
}
