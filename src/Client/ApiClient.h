#pragma once
#include <map>
#include <string>
#include <vector>
#include <httplib.h>
#include "ObjectStore.h"

struct PlateInfo {
    std::string id;
    std::string parent;
    std::string name;
    std::string flag;
    int fileCount = 0;
};

class ApiClient {
    ObjectStore& store;
public:
    std::string domain;
    std::string project;

    explicit ApiClient(ObjectStore& store,
                       std::string domain  = "http://localhost:8080",
                       std::string project = "");

    bool isServerOn();

    std::vector<std::string> checkBlobs(const std::vector<std::string>& hashes);
    std::string uploadBlob(const std::string& data);
    std::string createPlate(const std::string& parent,
                            const std::map<std::string, std::string>& tree,
                            const std::string& name = "");

    std::map<std::string, std::string> fetchLatestTree();
    std::map<std::string, std::string> fetchTree(const std::string& plateId);

    // Returns plate chain from HEAD → root.
    std::vector<PlateInfo> fetchLog();

    std::string downloadBlob(const std::string& hash);

    bool initProject(const std::string& name);
    bool deleteProject();
};
