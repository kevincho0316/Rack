#pragma once
#include <filesystem>
#include <string>
#include <vector>

class ObjectStore {
public:
    std::filesystem::path objectsDir;
    std::filesystem::path initFileDir;
    std::vector<std::string> newFile;

    ObjectStore(std::filesystem::path objDir, std::filesystem::path initDir);

    bool   exists(const std::string& hash) const;
    std::string read(const std::string& hash) const;
    std::string readInit() const;
    void   write(const std::string& hash, const std::string& data);
    void   writeInit(const std::string& hash);
};
