#pragma once
#include <filesystem>
#include <string>
#include <vector>

class ServerStorage {
    std::filesystem::path dataDir;

    std::filesystem::path projectDir(const std::string& p) const;
    std::filesystem::path blobsDir(const std::string& p) const;
    std::filesystem::path platesDir(const std::string& p) const;
    std::filesystem::path headFile(const std::string& p) const;

public:
    explicit ServerStorage(std::filesystem::path dataDir);

    void init();

    bool                     projectExists(const std::string& p) const;
    void                     createProject(const std::string& p);
    std::vector<std::string> listProjects() const;

    bool        blobExists(const std::string& p, const std::string& hash) const;
    std::string readBlob(const std::string& p, const std::string& hash) const;
    void        writeBlob(const std::string& p, const std::string& hash, const std::string& data);

    bool        plateExists(const std::string& p, const std::string& id) const;
    std::string readPlate(const std::string& p, const std::string& id) const;
    void        writePlate(const std::string& p, const std::string& id, const std::string& data);
    std::string readHead(const std::string& p) const;
    void        writeHead(const std::string& p, const std::string& id);
    std::vector<std::string> listPlateIds(const std::string& p) const;
};
