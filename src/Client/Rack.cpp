#include "Rack.h"
#include "Paths.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
#include "ProgressBar.h"

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
    auto loadJson = [](const fs::path& path, auto fn) {
        std::ifstream f(path);
        if (!f.is_open()) return;
        try { fn(json::parse(f)); } catch (...) {}
    };

    loadJson(RackPaths::globalConfigFile(), [&](const json& j) {
        if (j.contains("domain")) api.domain = j["domain"];
    });
    loadJson(RackPaths::configFile, [&](const json& j) {
        if (j.contains("project")) api.project = j["project"];
    });
}

void Rack::saveConfig() {
    auto globalDir = RackPaths::globalConfigFile().parent_path();
    if (!fs::exists(globalDir)) fs::create_directories(globalDir);

    std::ofstream g(RackPaths::globalConfigFile());
    g << json{{"domain", api.domain}}.dump(2);

    std::ofstream l(RackPaths::configFile);
    l << json{{"project", api.project}}.dump(2);
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

    if (!missing.empty()) {
        ProgressBar pb(static_cast<int>(missing.size()), "Uploading");
        for (const auto& hash : missing) {
            std::string data = store.read(hash);
            std::string serverHash = api.uploadBlob(data);
            if (serverHash.empty()) { std::cout << "\nUpload failed for " << hash << "\n"; return; }
            pb.tick();
        }
    }

    std::string plateId = api.createPlate(parentHash, flatTree, plateHash.substr(0, 8));
    if (!plateId.empty()) std::cout << "Pushed plate: " << plateId << "\n";
}

void Rack::pull(bool overwriteOnly) {
    if (api.project.empty()) {
        std::cout << "[WARN] No project set. Run: rack init <project>\n";
        return;
    }

    auto tree = api.fetchLatestTree();
    if (tree.empty()) { std::cout << "Nothing to pull\n"; return; }

    if (!overwriteOnly) {
        std::set<std::string> serverPaths;
        for (const auto& [path, hash] : tree) serverPaths.insert(path);

        std::vector<fs::path> toDelete;
        for (const auto& e : fs::recursive_directory_iterator(
                 fs::current_path(), fs::directory_options::skip_permission_denied)) {
            if (!e.is_regular_file()) continue;
            std::string rel = fs::relative(e.path(), fs::current_path()).string();
            if (rel.rfind(".rack", 0) == 0) continue;
            if (!serverPaths.count(rel)) toDelete.push_back(e.path());
        }
        for (const auto& p : toDelete) {
            fs::remove(p);
            std::cout << "Removed: " << fs::relative(p, fs::current_path()).string() << "\n";
        }
    }

    std::cout << "Pulling " << tree.size() << " files\n";
    ProgressBar pb(static_cast<int>(tree.size()), "Pulling ");
    for (const auto& [path, hash] : tree) {
        pb.tick();
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

void Rack::log() {
    if (api.project.empty()) { std::cout << "[WARN] No project set\n"; return; }
    auto chain = api.fetchLog();
    if (chain.empty()) { std::cout << "No plates yet\n"; return; }
    std::cout << "Project: " << api.project << "  (" << chain.size() << " plates)\n";
    for (size_t i = 0; i < chain.size(); ++i) {
        const auto& p = chain[i];
        std::string label = (i == 0) ? " <- HEAD" : "";
        std::cout << p.id.substr(0, 12)
                  << "  [" << p.flag << "]"
                  << "  \"" << p.name << "\""
                  << "  " << p.fileCount << " files"
                  << label << "\n";
    }
}

void Rack::files() {
    if (api.project.empty()) { std::cout << "[WARN] No project set\n"; return; }
    auto tree = api.fetchLatestTree();
    if (tree.empty()) { std::cout << "No files on server\n"; return; }
    std::cout << "Files in latest plate (" << tree.size() << "):\n";
    for (const auto& [path, hash] : tree)
        std::cout << "  " << path << "\n";
}

void Rack::status() {
    std::string plateHash = store.readInit();
    if (plateHash.empty()) { std::cout << "No local commits\n"; return; }

    std::string plateData = store.read(plateHash);
    std::string treeHash  = extractField(plateData, "[Tree]");
    if (treeHash.empty()) { std::cout << "Corrupt local plate\n"; return; }

    auto localTree  = reader.flattenTree(treeHash);
    auto serverTree = api.fetchLatestTree();

    if (serverTree.empty()) { std::cout << "Nothing on server yet\n"; return; }

    bool clean = true;
    for (const auto& [path, hash] : localTree) {
        if (!serverTree.count(path))           { std::cout << "  new:      " << path << "\n"; clean = false; }
        else if (serverTree.at(path) != hash)  { std::cout << "  modified: " << path << "\n"; clean = false; }
    }
    for (const auto& [path, hash] : serverTree)
        if (!localTree.count(path))            { std::cout << "  deleted:  " << path << "\n"; clean = false; }

    if (clean) std::cout << "Up to date with server\n";
}

void Rack::restore(const std::string& plateId) {
    if (api.project.empty()) { std::cout << "[WARN] No project set\n"; return; }
    auto tree = api.fetchTree(plateId);
    if (tree.empty()) { std::cout << "Plate not found or empty\n"; return; }

    std::set<std::string> serverPaths;
    for (const auto& [path, hash] : tree) serverPaths.insert(path);

    for (const auto& e : fs::recursive_directory_iterator(
             fs::current_path(), fs::directory_options::skip_permission_denied)) {
        if (!e.is_regular_file()) continue;
        std::string rel = fs::relative(e.path(), fs::current_path()).string();
        if (rel.rfind(".rack", 0) == 0) continue;
        if (!serverPaths.count(rel)) fs::remove(e.path());
    }

    ProgressBar pb(static_cast<int>(tree.size()), "Restoring");
    for (const auto& [path, hash] : tree) {
        pb.tick();
        if (path.rfind(".rack", 0) == 0) continue;
        std::string data = api.downloadBlob(hash);
        if (data.empty()) continue;
        fs::path fpath(path);
        if (fpath.has_parent_path()) fs::create_directories(fpath.parent_path());
        std::ofstream f(fpath, std::ios::binary);
        f << data;
    }
    std::cout << "Restored plate " << plateId.substr(0, 12) << "\n";
}

bool Rack::deleteProject() {
    return api.deleteProject();
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
