#include "RepoBuilder.h"
#include "model.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

RepoBuilder::RepoBuilder(ObjectStore& s) : store(s) {}

std::string RepoBuilder::createBlobFile(const std::string& content) {
    Blob blob;
    blob.content = content;
    std::string hash = blob.generate_blob_hash();
    if (store.exists(hash)) return hash;
    store.write(hash, blob.content);
    return hash;
}

std::string RepoBuilder::createTreeFile(const std::string& path) {
    std::vector<Branch> branches;
    std::vector<fs::directory_entry> entries;

    for (const auto& entry : fs::directory_iterator(path))
        entries.push_back(entry);

    std::sort(entries.begin(), entries.end(),
        [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.path().filename().string() < b.path().filename().string();
        });

    for (const auto& entry : entries) {
        if (entry.path().filename() == ".rack") continue;
        if (entry.is_directory()) {
            branches.emplace_back(true, entry.path().filename().string(),
                                  createTreeFile(entry.path().string()));
        } else {
            std::ifstream file(entry.path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            branches.emplace_back(false, entry.path().filename().string(),
                                  createBlobFile(content));
        }
    }

    Tree tree(branches);
    if (store.exists(tree.hash)) return tree.hash;
    store.write(tree.hash, tree.serialize());
    return tree.hash;
}

std::string RepoBuilder::createPlateFile() {
    std::string parentHash = store.readInit();
    std::string treeHash   = createTreeFile(fs::current_path().string());

    if (store.newFile.empty()) return "No Diff Found";

    Plate plate(parentHash, treeHash);
    if (store.exists(plate.hash)) return plate.hash;
    store.write(plate.hash, plate.serialize());
    store.writeInit(plate.hash);
    return plate.hash;
}
