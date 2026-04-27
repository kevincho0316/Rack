#include "Rack.h"
#include "Paths.h"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include "Color.h"
#include "ProgressBar.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

struct ScopedProject {
    std::string& ref;
    std::string  saved;
    ScopedProject(std::string& r, const std::string& override)
        : ref(r), saved(r) { if (!override.empty()) r = override; }
    ~ScopedProject() { ref = saved; }
};

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
        if (j.contains("apiKey")) api.apiKey = j["apiKey"];
    });
    loadJson(RackPaths::configFile, [&](const json& j) {
        if (j.contains("project"))       api.project   = j["project"];
        if (j.contains("serverPlateId")) serverPlateId = j["serverPlateId"];
    });
}

void Rack::saveGlobalConfig() {
    auto globalDir = RackPaths::globalConfigFile().parent_path();
    if (!fs::exists(globalDir)) fs::create_directories(globalDir);
    json j;
    if (!api.domain.empty()) j["domain"] = api.domain;
    if (!api.apiKey.empty()) j["apiKey"] = api.apiKey;
    std::ofstream g(RackPaths::globalConfigFile());
    g << j.dump(2);
}

void Rack::saveConfig() {
    saveGlobalConfig();
    std::ofstream l(RackPaths::configFile);
    l << json{{"project", api.project}, {"serverPlateId", serverPlateId}}.dump(2);
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

std::string Rack::commit(const std::string& name, const std::string& flag) {
    return builder.createPlateFile(name, flag);
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
    saveGlobalConfig();
}

void Rack::setApiKey(const std::string& key) {
    api.apiKey = key;
    saveGlobalConfig();
}

bool Rack::isServerOn() {
    return api.isServerOn();
}

void Rack::push(const std::string& proj) {
    ScopedProject sp(api.project, proj);
    bool isOverride = !proj.empty() && proj != sp.saved;

    if (api.project.empty()) {
        std::cout << Color::y("[WARN]") << " No project set. Run: rack init <project>\n";
        return;
    }

    std::string plateHash = store.readInit();
    if (plateHash.empty()) { std::cout << Color::dim("Nothing to push — no commits") << "\n"; return; }

    std::string plateData  = store.read(plateHash);
    std::string treeHash   = extractField(plateData, "[Tree]");

    auto flatTree = reader.flattenTree(treeHash);
    if (flatTree.empty()) { std::cout << Color::dim("Empty tree — nothing to push") << "\n"; return; }

    std::vector<std::string> allHashes;
    for (const auto& [path, hash] : flatTree) allHashes.push_back(hash);

    std::vector<std::string> missing = api.checkBlobs(allHashes);
    std::cout << Color::c("Blobs:") << " " << allHashes.size() << " total, "
              << missing.size() << " to upload\n";

    if (!missing.empty()) {
        ProgressBar pb(static_cast<int>(missing.size()), "Uploading");
        for (const auto& hash : missing) {
            std::string data = store.read(hash);
            std::string serverHash = api.uploadBlob(data);
            if (serverHash.empty()) { std::cout << "\n" << Color::r("Upload failed for ") << hash << "\n"; return; }
            pb.tick();
        }
    }

    std::string plateName = extractField(plateData, "[Name]");
    std::string plateFlag = extractField(plateData, "[Flag]");
    if (plateFlag.empty()) plateFlag = "Normal";

    std::string parentId = isOverride ? "" : serverPlateId;
    std::string plateId  = api.createPlate(parentId, flatTree, plateName, plateFlag);
    if (!plateId.empty()) {
        if (!isOverride) { serverPlateId = plateId; saveConfig(); }
        std::cout << Color::gb("Pushed plate:") << " " << plateId << "\n";
    }
}

void Rack::pull(bool overwriteOnly, const std::string& proj) {
    ScopedProject sp(api.project, proj);
    if (api.project.empty()) {
        std::cout << Color::y("[WARN]") << " No project set. Run: rack init <project>\n";
        return;
    }

    auto tree = api.fetchLatestTree();
    if (tree.empty()) { std::cout << Color::dim("Nothing to pull") << "\n"; return; }

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
            std::cout << Color::r("Removed:") << " " << fs::relative(p, fs::current_path()).string() << "\n";
        }
    }

    std::cout << Color::c("Pulling") << " " << tree.size() << " files\n";
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
    std::cout << Color::gb("Pull complete") << "\n";
}

void Rack::log(const std::string& proj) {
    ScopedProject sp(api.project, proj);
    if (api.project.empty()) { std::cout << Color::y("[WARN]") << " No project set\n"; return; }
    auto chain = api.fetchLog();
    if (chain.empty()) { std::cout << "No plates yet\n"; return; }
    std::cout << "Project: " << Color::b(api.project)
              << Color::dim("  (" + std::to_string(chain.size()) + " plates)") << "\n";
    for (size_t i = 0; i < chain.size(); ++i) {
        const auto& p = chain[i];
        std::string timeStr = "unknown";
        if (p.uploadedAt != 0) {
            std::time_t t = static_cast<std::time_t>(p.uploadedAt);
            std::tm* tm = std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
            timeStr = oss.str();
        }
        std::string flagStr;
        if (p.flag == "Hotfix") flagStr = Color::r("[Hotfix]");
        else if (p.flag == "Knot") flagStr = Color::m("[Knot]");
        else flagStr = Color::dim("[Normal]");
        std::string nameStr = p.name.empty() ? Color::dim("\"\"") : Color::y("\"" + p.name + "\"");
        std::string headStr = (i == 0) ? "  " + Color::gb("<- HEAD") : "";
        std::cout << Color::dim(p.id.substr(0, 12))
                  << "  " << flagStr
                  << "  " << nameStr
                  << "  " << Color::dim(std::to_string(p.fileCount) + " files")
                  << "  " << Color::dim(timeStr)
                  << headStr << "\n";
    }
}

void Rack::files(const std::string& proj) {
    ScopedProject sp(api.project, proj);
    if (api.project.empty()) { std::cout << Color::y("[WARN]") << " No project set\n"; return; }
    auto tree = api.fetchLatestTree();
    if (tree.empty()) { std::cout << "No files on server\n"; return; }
    std::cout << "Files in latest plate (" << tree.size() << "):\n";
    for (const auto& [path, hash] : tree)
        std::cout << "  " << path << "\n";
}

void Rack::projects() {
    auto list = api.listProjects();
    if (list.empty()) { std::cout << "No projects on server\n"; return; }
    std::cout << "Projects (" << list.size() << "):\n";
    for (const auto& p : list)
        std::cout << "  " << p << (p == api.project ? "  <- active" : "") << "\n";
}

void Rack::status(const std::string& proj) {
    ScopedProject sp(api.project, proj);
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
        if (!serverTree.count(path))           { std::cout << "  " << Color::g("new:     ") << " " << path << "\n"; clean = false; }
        else if (serverTree.at(path) != hash)  { std::cout << "  " << Color::y("modified:") << " " << path << "\n"; clean = false; }
    }
    for (const auto& [path, hash] : serverTree)
        if (!localTree.count(path))            { std::cout << "  " << Color::r("deleted: ") << " " << path << "\n"; clean = false; }

    if (clean) std::cout << Color::gb("Up to date with server") << "\n";
}

void Rack::restore(const std::string& plateId, const std::string& proj) {
    ScopedProject sp(api.project, proj);
    if (api.project.empty()) { std::cout << Color::y("[WARN]") << " No project set\n"; return; }
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
    std::cout << Color::gb("Restored plate") << " " << plateId.substr(0, 12) << "\n";
}

// ── diff helpers ────────────────────────────────────────────────────────────

static std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> lines;
    size_t start = 0, pos;
    while ((pos = s.find('\n', start)) != std::string::npos) {
        lines.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    lines.push_back(s.substr(start));
    return lines;
}

static bool isBinary(const std::string& s) {
    return s.find('\0') != std::string::npos;
}

static void printUnifiedDiff(const std::string& path,
                              const std::string& contA,
                              const std::string& contB) {
    std::string headerA = contA.empty() ? "/dev/null" : "a/" + path;
    std::string headerB = contB.empty() ? "/dev/null" : "b/" + path;
    std::cout << Color::dim("--- " + headerA) << "\n"
              << Color::dim("+++ " + headerB) << "\n";

    if (isBinary(contA) || isBinary(contB)) {
        std::cout << "  Binary files differ\n";
        return;
    }

    auto a = splitLines(contA);
    auto b = splitLines(contB);
    if (contA.empty()) a.clear();
    if (contB.empty()) b.clear();

    int m = (int)a.size(), n = (int)b.size();

    if (m > 2000 || n > 2000) {
        std::cout << "@@ file too large (" << m << " vs " << n << " lines) @@\n";
        return;
    }

    // LCS DP
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++)
            dp[i][j] = (a[i-1] == b[j-1]) ? dp[i-1][j-1] + 1
                                            : std::max(dp[i-1][j], dp[i][j-1]);

    // Backtrack into edit ops: -1=removed, +1=added, 0=context
    std::vector<std::pair<int, std::string>> ops;
    int i = m, j = n;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && a[i-1] == b[j-1]) {
            ops.push_back({0, a[i-1]}); i--; j--;
        } else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
            ops.push_back({1, b[j-1]}); j--;
        } else {
            ops.push_back({-1, a[i-1]}); i--;
        }
    }
    std::reverse(ops.begin(), ops.end());

    // Mark lines to show (changed ± 3 context)
    int sz = (int)ops.size();
    std::vector<bool> show(sz, false);
    for (int k = 0; k < sz; k++) {
        if (ops[k].first != 0) {
            for (int c = std::max(0, k - 3); c <= std::min(sz - 1, k + 3); c++)
                show[c] = true;
        }
    }

    bool inHunk = false;
    for (int k = 0; k < sz; k++) {
        if (!show[k]) { inHunk = false; continue; }
        if (!inHunk) { std::cout << Color::c("@@ ... @@") << "\n"; inHunk = true; }
        auto& ln = ops[k].second;
        if      (ops[k].first == -1) std::cout << Color::r("-" + ln) << "\n";
        else if (ops[k].first ==  1) std::cout << Color::g("+" + ln) << "\n";
        else                         std::cout << " " << ln << "\n";
    }
}

void Rack::diff(const std::string& plateIdA, const std::string& plateIdB) {
    if (!api.isServerOn()) { std::cout << "Server offline\n"; return; }

    // Resolve short prefix → full plate ID
    auto resolveId = [&](const std::string& id) -> std::string {
        if (id.size() == 64) return id;
        auto chain = api.fetchLog();
        for (auto& pi : chain)
            if (pi.id.rfind(id, 0) == 0) return pi.id;
        std::cout << "Plate not found: " << id << "\n";
        return id;
    };

    bool aIsLocal = plateIdA.empty();

    std::map<std::string, std::string> treeA, treeB;
    std::string labelA, labelB;

    if (aIsLocal) {
        std::string plateHash = store.readInit();
        if (plateHash.empty()) { std::cout << "No local commits\n"; return; }
        std::string treeHash = extractField(store.read(plateHash), "[Tree]");
        treeA  = reader.flattenTree(treeHash);
        labelA = "local";
    } else {
        std::string fullA = resolveId(plateIdA);
        treeA  = api.fetchTree(fullA);
        labelA = fullA.substr(0, 12);
    }

    if (plateIdB.empty()) {
        treeB  = api.fetchLatestTree();
        labelB = "server:HEAD";
    } else {
        std::string fullB = resolveId(plateIdB);
        treeB  = api.fetchTree(fullB);
        labelB = fullB.substr(0, 12);
    }

    std::cout << "diff " << labelA << " → " << labelB << "\n";

    std::set<std::string> allPaths;
    for (auto& [p, h] : treeA) allPaths.insert(p);
    for (auto& [p, h] : treeB) allPaths.insert(p);

    auto fetchContent = [&](const std::string& hash, bool preferLocal) -> std::string {
        if (preferLocal) {
            std::string s = store.read(hash);
            if (!s.empty()) return s;
        }
        return api.downloadBlob(hash);
    };

    int added = 0, removed = 0, modified = 0;

    for (const auto& path : allPaths) {
        bool inA = treeA.count(path), inB = treeB.count(path);
        if (inA && inB && treeA.at(path) == treeB.at(path)) continue;

        std::cout << "\n";
        if (!inA) {
            ++added;
            std::string cont = fetchContent(treeB.at(path), true);
            printUnifiedDiff(path, "", cont);
        } else if (!inB) {
            ++removed;
            std::string cont = fetchContent(treeA.at(path), aIsLocal);
            printUnifiedDiff(path, cont, "");
        } else {
            ++modified;
            std::string contA = fetchContent(treeA.at(path), aIsLocal);
            std::string contB = fetchContent(treeB.at(path), true);
            printUnifiedDiff(path, contA, contB);
        }
    }

    if (added == 0 && removed == 0 && modified == 0)
        std::cout << "No differences\n";
    else
        std::cout << "\n" << added << " added, " << removed << " removed, "
                  << modified << " modified\n";
}

// ────────────────────────────────────────────────────────────────────────────

bool Rack::deleteProject() {
    return api.deleteProject();
}

bool Rack::initProject(const std::string& name) {
    bool ok = api.initProject(name);
    if (ok) {
        api.project = name;
        saveConfig();
        std::cout << Color::g("Project '") << name << Color::g("' active") << "\n";
    }
    return ok;
}
