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

int pull(Rack& rack) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.pull();
    return 0;
}

int init(Rack& rack, const std::string& project) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    return rack.initProject(project) ? 0 : 1;
}

} // namespace Commands
