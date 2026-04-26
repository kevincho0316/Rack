#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "ObjectStore.h"
#include "RepoBuilder.h"
#include "RepoReader.h"
#include "ApiClient.h"

class Rack {
    std::string serverPlateId;  // last pushed server plate ID, used as parent for next push

    void loadConfig();
    void saveConfig();

public:
    ObjectStore store;
    RepoBuilder builder;
    RepoReader  reader;
    ApiClient   api;

    Rack();

    std::string              commit(const std::string& name = "",
                                   const std::string& flag = "Normal");
    std::string              readFile(const std::string& hash);
    std::vector<std::string> ls();
    void                     reconstruct(const std::filesystem::path& dest);
    void                     setDomain(const std::string& domain);
    bool                     isServerOn();

    void push(const std::string& proj = "");
    void pull(bool overwriteOnly = false, const std::string& proj = "");
    bool initProject(const std::string& name);
    bool deleteProject();

    void log(const std::string& proj = "");
    void files(const std::string& proj = "");
    void projects();
    void status(const std::string& proj = "");
    void restore(const std::string& plateId, const std::string& proj = "");
    void diff(const std::string& plateIdA = "", const std::string& plateIdB = "");
};
