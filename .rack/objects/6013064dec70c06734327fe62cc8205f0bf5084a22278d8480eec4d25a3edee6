#pragma once
#include <string>
#include "ObjectStore.h"

class RepoBuilder {
    ObjectStore& store;
public:
    explicit RepoBuilder(ObjectStore& store);

    std::string createBlobFile(const std::string& content);
    std::string createTreeFile(const std::string& path);
    std::string createPlateFile();
};
