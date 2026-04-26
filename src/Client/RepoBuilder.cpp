#include "RepoBuilder.h"
#include "model.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

static int countFiles(const fs::path& root) {
    int n = 0;
    for (const auto& e : fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied)) {
        if (!e.is_regular_file()) continue;
        auto rel = fs::relative(e.path(), root);
        if (rel.begin() != rel.end() && rel.begin()->string() == ".rack") continue;
        ++n;
    }
    return n;
}

RepoBuilder::RepoBuilder(ObjectStore& s) : store(s) {}

std::string RepoBuilder::createBlobFile(const std::string& content) {
    Blob blob;
    blob.content = content;
    std::string hash = blob.generate_blob_hash();
    if (store.exists(hash)) return hash;
    store.write(hash, blob.content);
    return hash;
}

std::string RepoBuilder::createTreeFileImpl(const std::string& path, ProgressBar* pb) {
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
                                  createTreeFileImpl(entry.path().string(), pb));
        } else {
            std::ifstream file(entry.path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            branches.emplace_back(false, entry.path().filename().string(),
                                  createBlobFile(content));
            if (pb) pb->tick();
        }
    }

    Tree tree(branches);
    if (store.exists(tree.hash)) return tree.hash;
    store.write(tree.hash, tree.serialize());
    return tree.hash;
}

std::string RepoBuilder::createTreeFile(const std::string& path) {
    return createTreeFileImpl(path, nullptr);
}

std::string RepoBuilder::createPlateFile(const std::string& name, const std::string& flag) {
    std::string parentHash = store.readInit();

    int total = countFiles(fs::current_path());
    std::cout << "Scanning " << total << " files\n";
    ProgressBar pb(total, "Hashing ");
    std::string treeHash = createTreeFileImpl(fs::current_path().string(), &pb);

    if (store.newFile.empty()) return "No Diff Found";

    Plate plate(parentHash, treeHash, name, flag);
    if (store.exists(plate.hash)) return plate.hash;
    store.write(plate.hash, plate.serialize());
    store.writeInit(plate.hash);
    return plate.hash;
}
