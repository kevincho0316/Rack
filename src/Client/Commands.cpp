#include "Commands.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace Commands {

int commit(Rack& rack) {
    std::string result = rack.commit();
    std::cout << "Local commit: " << result << "\n";
    if (result == "No Diff Found") return 0;
    if (!rack.api.project.empty() && rack.isServerOn()) {
        rack.push();
    } else if (!rack.api.project.empty()) {
        std::cout << "[WARN] Server offline — local commit saved, push skipped\n";
    }
    return 0;
}

int cat(Rack& rack, const std::string& hash) {
    std::cout << rack.readFile(hash) << "\n";
    return 0;
}

int ls(Rack& rack) {
    for (const auto& h : rack.ls()) std::cout << h << "\n";
    return 0;
}

int setDomain(Rack& rack, const std::string& domain) {
    rack.setDomain(domain);
    std::cout << "Domain set: " << domain << "\n";
    return 0;
}

int reconstruct(Rack& rack) {
    rack.reconstruct(fs::current_path());
    return 0;
}

int serverCheck(Rack& rack) {
    return rack.isServerOn() ? 0 : 1;
}

int push(Rack& rack) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.push();
    return 0;
}

int pull(Rack& rack, bool overwriteOnly) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.pull(overwriteOnly);
    return 0;
}

int init(Rack& rack, const std::string& project) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    return rack.initProject(project) ? 0 : 1;
}

int log(Rack& rack) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.log();
    return 0;
}

int files(Rack& rack) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.files();
    return 0;
}

int status(Rack& rack) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.status();
    return 0;
}

int restore(Rack& rack, const std::string& plateId) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.restore(plateId);
    return 0;
}

int deleteProject(Rack& rack) {
    if (rack.api.project.empty()) { std::cout << "No project set\n"; return 1; }
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    std::cout << "Delete project '" << rack.api.project << "' from server. Cannot be undone.\n";
    std::cout << "Type project name to confirm: ";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != rack.api.project) { std::cout << "Cancelled\n"; return 1; }
    bool ok = rack.deleteProject();
    if (ok) std::cout << "Deleted '" << rack.api.project << "'\n";
    else    std::cout << "Delete failed\n";
    return ok ? 0 : 1;
}

} // namespace Commands
