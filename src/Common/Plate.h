#pragma once
#include <string>
#include "hash.h"

struct Plate {
    std::string hash;
    std::string parent_hash;
    std::string init_Tree_hash;
    std::string name;
    std::string flag = "Normal";

    std::string serialize() const {
        return "[Parent]" + parent_hash + "\n"
             + "[Tree]"   + init_Tree_hash + "\n"
             + "[Name]"   + name + "\n"
             + "[Flag]"   + flag + "\n";
    }

    Plate(std::string parent, std::string tree,
          std::string name = "", std::string flag = "Normal")
        : parent_hash(parent), init_Tree_hash(tree), name(name), flag(flag) {
        hash = GetHash(serialize());
    }
};
