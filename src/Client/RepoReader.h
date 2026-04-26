#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "ObjectStore.h"

class RepoReader {
    ObjectStore& store;
public:
    explicit RepoReader(ObjectStore& store);

    std::string getInitPlate();
    std::vector<std::string> ExtractHash(const std::string& initHash, const std::string& startDelim);
    std::unordered_map<std::string, std::string> GetHashNameMap(const std::string& hash, bool isFile);
    void reconstruct(const std::string& initHash, std::filesystem::path destPath);

    // Returns flat {path -> blobHash} map for a tree hash.
    std::map<std::string, std::string> flattenTree(const std::string& treeHash,
                                                   const std::string& prefix = "");
};
