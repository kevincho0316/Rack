#include "Commands.h"
#include <filesystem>
#include <iostream>
#include "Color.h"

namespace fs = std::filesystem;

namespace {
struct ScopedProject {
    std::string& ref; std::string saved;
    ScopedProject(std::string& r, const std::string& o) : ref(r), saved(r) { if (!o.empty()) r = o; }
    ~ScopedProject() { ref = saved; }
};
}

namespace Commands {

int commit(Rack& rack, const std::string& name, const std::string& flag) {
    std::string result = rack.commit(name, flag);
    std::cout << Color::gb("Local commit:") << " " << result << "\n";
    if (result == "No Diff Found" && name=="" && flag == "") return 0;
    if (!rack.api.project.empty() && rack.isServerOn()) {
        rack.push();
    } else if (!rack.api.project.empty()) {
        std::cout << Color::y("[WARN]") << " Server offline — local commit saved, push skipped\n";
    }
    return 0;
}

int setApiKey(Rack& rack, const std::string& key) {
    rack.setApiKey(key);
    std::cout << Color::g("API key saved") << " to ~/.rack/config\n";
    return 0;
}

int checkout(Rack& rack, const std::string& domain, const std::string& project) {
    rack.setDomain(domain);
    std::cout << Color::c("Domain:") << " " << domain << "\n";
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline\n"); return 1; }
    if (!rack.initProject(project)) return 1;
    rack.pull(false, "");
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
    std::cout << Color::g("Domain set:") << " " << domain << "\n";
    return 0;
}

int reconstruct(Rack& rack) {
    rack.reconstruct(fs::current_path());
    return 0;
}

int serverCheck(Rack& rack) {
    return rack.isServerOn() ? 0 : 1;
}

int push(Rack& rack, const std::string& proj) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    rack.push(proj);
    return 0;
}

int pull(Rack& rack, bool overwriteOnly, const std::string& proj) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    rack.pull(overwriteOnly, proj);
    return 0;
}

int init(Rack& rack, const std::string& project) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    return rack.initProject(project) ? 0 : 1;
}

int log(Rack& rack, const std::string& proj) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    rack.log(proj);
    return 0;
}

int files(Rack& rack, const std::string& proj) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    rack.files(proj);
    return 0;
}

int projects(Rack& rack) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    rack.projects();
    return 0;
}

int status(Rack& rack, const std::string& proj) {
    if (!rack.isServerOn()) { std::cout << Color::y("Server offline") << "\n"; return 1; }
    rack.status(proj);
    return 0;
}

int restore(Rack& rack, const std::string& plateId, const std::string& proj) {
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    rack.restore(plateId, proj);
    return 0;
}

int deleteProject(Rack& rack, const std::string& proj, bool autoConfirm) {
    ScopedProject sp(rack.api.project, proj);
    if (rack.api.project.empty()) { std::cout << "No project set\n"; return 1; }
    if (!rack.isServerOn()) { std::cout << "Server offline\n"; return 1; }
    if (!autoConfirm) {
        std::cout << "Delete '" << rack.api.project << "' from server? [y/N] ";
        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm != "y" && confirm != "Y") { std::cout << "Cancelled\n"; return 1; }
    }
    bool ok = rack.deleteProject();
    if (ok) std::cout << "Deleted '" << rack.api.project << "'\n";
    else    std::cout << "Delete failed\n";
    return ok ? 0 : 1;
}

int diff(Rack& rack, const std::string& plateIdA, const std::string& plateIdB) {
    rack.diff(plateIdA, plateIdB);
    return 0;
}

} // namespace Commands
