#pragma once
#include <map>
#include <string>
#include <vector>
#include <httplib.h>
#include "ObjectStore.h"

class ApiClient {
    ObjectStore& store;
public:
    std::string domain;
    std::string project;

    explicit ApiClient(ObjectStore& store,
                       std::string domain  = "http://localhost:8080",
                       std::string project = "");

    bool isServerOn();

    // Returns missing hashes from server.
    std::vector<std::string> checkBlobs(const std::vector<std::string>& hashes);

    // Uploads one blob as raw bytes. Returns server hash.
    std::string uploadBlob(const std::string& data);

    // Creates plate on server. Returns server-assigned plate id.
    std::string createPlate(const std::string& parent,
                            const std::map<std::string, std::string>& tree,
                            const std::string& name = "");

    // Downloads latest plate tree from server: {path -> hash}.
    std::map<std::string, std::string> fetchLatestTree();

    // Downloads blob by hash, writes to local store. Returns data.
    std::string downloadBlob(const std::string& hash);

    // Creates project on server.
    bool initProject(const std::string& name);
};
