#pragma once
#include <string>
#include "hash.h"

enum PlateFlag { Normal, Knot };

struct Plate {
    std::string hash;
    std::string parent_hash;
    std::string init_Tree_hash;
    PlateFlag flag = Normal;

    std::string serialize() const {
        return "[Parent]" + parent_hash + "\n" + "[Tree]" + init_Tree_hash + "\n";
    }

    Plate(std::string parent, std::string tree)
        : parent_hash(parent), init_Tree_hash(tree) {
        hash = GetHash(serialize());
    }
};
