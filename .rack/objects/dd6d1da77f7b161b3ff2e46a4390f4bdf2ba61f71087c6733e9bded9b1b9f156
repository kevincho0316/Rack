#include "RepoReader.h"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

RepoReader::RepoReader(ObjectStore& s) : store(s) {}

std::string RepoReader::getInitPlate() {
    std::string plateHash = store.readInit();
    auto treeInit = ExtractHash(plateHash, "[Tree]");
    if (treeInit.size() < 2) { std::cout << "No commits yet\n"; return ""; }
    return treeInit[1];
}

std::vector<std::string> RepoReader::ExtractHash(const std::string& initHash,
                                                  const std::string& startDelim) {
    std::vector<std::string> results;
    std::vector<std::string> waitingLists;
    const std::string endDelim = "\n";

    waitingLists.push_back(initHash);

    while (!waitingLists.empty()) {
        std::string current = waitingLists[0];
        waitingLists.erase(waitingLists.begin());
        results.push_back(current);

        std::string content = store.read(current);
        if (content.empty()) continue;

        size_t pos = 0;
        while ((pos = content.find(startDelim, pos)) != std::string::npos) {
            size_t startPos = pos + startDelim.length();
            size_t endPos   = content.find(endDelim, startPos);
            if (endPos != std::string::npos) {
                waitingLists.push_back(content.substr(startPos, endPos - startPos));
                pos = endPos;
            } else break;
        }
    }
    return results;
}

std::unordered_map<std::string, std::string>
RepoReader::GetHashNameMap(const std::string& hash, bool isFile) {
    const std::string prefix    = isFile ? "[File]" : "[Dir]";
    const std::string hashDelim = prefix + "[Hash]";
    const std::string nameDelim = prefix + "[Name]";
    std::unordered_map<std::string, std::string> result;

    std::string content = store.read(hash);
    if (content.empty()) return result;

    size_t pos = 0;
    while ((pos = content.find(hashDelim, pos)) != std::string::npos) {
        size_t hashStart = pos + hashDelim.size();
        size_t hashEnd   = content.find('\n', hashStart);
        if (hashEnd == std::string::npos) break;
        std::string entryHash = content.substr(hashStart, hashEnd - hashStart);

        size_t namePos = content.find(nameDelim, hashEnd);
        if (namePos == std::string::npos) break;
        size_t nameStart = namePos + nameDelim.size();
        size_t nameEnd   = content.find('\n', nameStart);
        std::string entryName = (nameEnd == std::string::npos)
            ? content.substr(nameStart)
            : content.substr(nameStart, nameEnd - nameStart);

        result[entryHash] = entryName;
        pos = hashEnd;
    }
    return result;
}

void RepoReader::reconstruct(const std::string& initHash, fs::path destPath) {
    if (!fs::exists(destPath)) fs::create_directories(destPath);

    auto fileMap = GetHashNameMap(initHash, true);
    auto dirMap  = GetHashNameMap(initHash, false);

    for (const auto& [hash, name] : fileMap) {
        std::ofstream f(destPath / name, std::ios::binary);
        f << store.read(hash);
    }

    std::vector<fs::directory_entry> entries;
    for (const auto& entry : fs::directory_iterator(destPath))
        entries.push_back(entry);

    std::sort(entries.begin(), entries.end(),
        [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.path().filename().string() < b.path().filename().string();
        });

    for (const auto& entry : entries) {
        if (entry.path().filename() == ".rack") continue;
        std::string name = entry.path().filename().string();
        if (entry.is_directory()) {
            auto it = std::find_if(dirMap.begin(), dirMap.end(),
                [&name](const auto& p) { return p.second == name; });
            if (it == dirMap.end()) fs::remove_all(entry.path());
        } else {
            auto it = std::find_if(fileMap.begin(), fileMap.end(),
                [&name](const auto& p) { return p.second == name; });
            if (it == fileMap.end()) fs::remove(entry.path());
        }
    }

    for (const auto& [hash, name] : dirMap) {
        if (name == ".rack") continue;
        reconstruct(hash, destPath / name);
    }
}

std::map<std::string, std::string> RepoReader::flattenTree(const std::string& treeHash,
                                                            const std::string& prefix) {
    std::map<std::string, std::string> result;
    if (treeHash.empty()) return result;

    std::string content = store.read(treeHash);
    if (content.empty()) return result;

    // Tree format per entry (two consecutive lines):
    //   [Dir][Hash]<hash>\n[Dir][Name]<name>\n
    //   [File][Hash]<hash>\n[File][Name]<name>\n
    size_t pos = 0;
    while (pos < content.size()) {
        bool isDir;
        std::string hashTag, nameTag;

        if (content.compare(pos, 11, "[Dir][Hash]") == 0) {
            isDir = true; hashTag = "[Dir][Hash]"; nameTag = "[Dir][Name]";
        } else if (content.compare(pos, 12, "[File][Hash]") == 0) {
            isDir = false; hashTag = "[File][Hash]"; nameTag = "[File][Name]";
        } else {
            size_t nl = content.find('\n', pos);
            pos = (nl == std::string::npos) ? content.size() : nl + 1;
            continue;
        }

        size_t hashStart = pos + hashTag.size();
        size_t hashEnd   = content.find('\n', hashStart);
        if (hashEnd == std::string::npos) break;
        std::string h = content.substr(hashStart, hashEnd - hashStart);

        size_t nameLineStart = hashEnd + 1;
        if (nameLineStart >= content.size()) break;
        if (content.compare(nameLineStart, nameTag.size(), nameTag) != 0) {
            pos = nameLineStart;
            continue;
        }
        size_t nameStart = nameLineStart + nameTag.size();
        size_t nameEnd   = content.find('\n', nameStart);
        std::string n = (nameEnd == std::string::npos)
            ? content.substr(nameStart)
            : content.substr(nameStart, nameEnd - nameStart);

        if (isDir) {
            auto sub = flattenTree(h, prefix + n + "/");
            result.insert(sub.begin(), sub.end());
        } else {
            result[prefix + n] = h;
        }

        pos = (nameEnd == std::string::npos) ? content.size() : nameEnd + 1;
    }
    return result;
}
