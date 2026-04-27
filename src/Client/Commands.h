#pragma once
#include <string>
#include "Rack.h"

namespace Commands {
    int commit(Rack& rack, const std::string& name = "",
                           const std::string& flag = "Normal");
    int cat(Rack& rack, const std::string& hash);
    int ls(Rack& rack);
    int setDomain(Rack& rack, const std::string& domain);
    int reconstruct(Rack& rack);
    int serverCheck(Rack& rack);
    int push(Rack& rack, const std::string& proj = "");
    int pull(Rack& rack, bool overwriteOnly = false, const std::string& proj = "");
    int init(Rack& rack, const std::string& project);
    int log(Rack& rack, const std::string& proj = "");
    int files(Rack& rack, const std::string& proj = "");
    int projects(Rack& rack);
    int deleteProject(Rack& rack, const std::string& proj = "", bool autoConfirm = false);
    int status(Rack& rack, const std::string& proj = "");
    int restore(Rack& rack, const std::string& plateId, const std::string& proj = "");
    int diff(Rack& rack, const std::string& plateIdA = "", const std::string& plateIdB = "");
    int checkout(Rack& rack, const std::string& domain, const std::string& project);
    int setApiKey(Rack& rack, const std::string& key);
}
