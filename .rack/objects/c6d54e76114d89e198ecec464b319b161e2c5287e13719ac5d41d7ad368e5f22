#pragma once
#include <sstream>
#include <string>
#include <vector>
#include "hash.h"

struct Branch {
    bool isDirectory;
    std::string name;
    std::string hash;
    Branch(bool d, std::string n, std::string h) : isDirectory(d), name(n), hash(h) {}
};

struct Tree {
    std::vector<Branch> Branches;
    std::string hash;

    std::string serialize() const {
        std::ostringstream result;
        for (auto& l : Branches) {
            result << (l.isDirectory ? "[Dir]" : "[File]");
            result << "[Hash]" + l.hash + "\n"
                   + (l.isDirectory ? "[Dir]" : "[File]")
                   + "[Name]" + l.name + "\n";
        }
        return result.str();
    }

    Tree(std::vector<Branch>& b) : Branches(b) {
        hash = GetHash(serialize());
    }
};
