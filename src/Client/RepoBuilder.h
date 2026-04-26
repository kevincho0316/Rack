#pragma once
#include <string>
#include "ObjectStore.h"
#include "ProgressBar.h"

class RepoBuilder {
    ObjectStore& store;

    std::string createTreeFileImpl(const std::string& path, ProgressBar* pb);
public:
    explicit RepoBuilder(ObjectStore& store);

    std::string createBlobFile(const std::string& content);
    std::string createTreeFile(const std::string& path);
    std::string createPlateFile();
};
