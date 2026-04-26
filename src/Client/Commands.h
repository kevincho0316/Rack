#pragma once
#include <string>
#include "Rack.h"

namespace Commands {
    int commit(Rack& rack);
    int cat(Rack& rack, const std::string& hash);
    int ls(Rack& rack);
    int setDomain(Rack& rack, const std::string& domain);
    int reconstruct(Rack& rack);
    int serverCheck(Rack& rack);
    int push(Rack& rack);
    int pull(Rack& rack, bool overwriteOnly = false);
    int init(Rack& rack, const std::string& project);
    int log(Rack& rack);
    int files(Rack& rack);
    int deleteProject(Rack& rack);
    int status(Rack& rack);
    int restore(Rack& rack, const std::string& plateId);
}
