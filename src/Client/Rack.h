#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "ObjectStore.h"
#include "RepoBuilder.h"
#include "RepoReader.h"
#include "ApiClient.h"

class Rack {
    void loadConfig();
    void saveConfig();

public:
    ObjectStore store;
    RepoBuilder builder;
    RepoReader  reader;
    ApiClient   api;

    Rack();

    std::string              commit();
    std::string              readFile(const std::string& hash);
    std::vector<std::string> ls();
    void                     reconstruct(const std::filesystem::path& dest);
    void                     setDomain(const std::string& domain);
    bool                     isServerOn();

    void push();
    void pull(bool overwriteOnly = false);
    bool initProject(const std::string& name);
    bool deleteProject();

    void log();
    void files();
    void status();
    void restore(const std::string& plateId);
};
