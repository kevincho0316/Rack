#include "ApiClient.h"
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ApiClient::ApiClient(ObjectStore& s, std::string d, std::string p)
    : store(s), domain(std::move(d)), project(std::move(p)) {}

bool ApiClient::isServerOn() {
    httplib::Client cli(domain);
    auto res = cli.Get("/hello");
    if (!res) { std::cout << "Server unreachable: " << res.error() << "\n"; return false; }
    return res->status == 200;
}

std::vector<std::string> ApiClient::checkBlobs(const std::vector<std::string>& hashes) {
    httplib::Client cli(domain);
    json body = {{"hashes", hashes}};
    auto res = cli.Post("/projects/" + project + "/blobs/check",
                        body.dump(), "application/json");
    if (!res || res->status != 200) {
        std::cout << "checkBlobs failed\n";
        return {};
    }
    auto j = json::parse(res->body);
    return j["missing"].get<std::vector<std::string>>();
}

std::string ApiClient::uploadBlob(const std::string& data) {
    httplib::Client cli(domain);
    auto res = cli.Post("/projects/" + project + "/blobs", data, "application/octet-stream");
    if (!res || res->status != 201) {
        std::cout << "uploadBlob failed\n";
        return "";
    }
    return json::parse(res->body)["hash"].get<std::string>();
}

std::string ApiClient::createPlate(const std::string& parent,
                                   const std::map<std::string, std::string>& tree,
                                   const std::string& name,
                                   const std::string& flag) {
    httplib::Client cli(domain);
    json treeJson = json::object();
    for (const auto& [path, hash] : tree) treeJson[path] = hash;
    json body = {{"parent", parent}, {"name", name}, {"flag", flag}, {"tree", treeJson}};
    auto res = cli.Post("/projects/" + project + "/plates",
                        body.dump(), "application/json");
    if (!res || res->status != 201) {
        std::cout << "createPlate failed\n";
        return "";
    }
    return json::parse(res->body)["id"].get<std::string>();
}

std::map<std::string, std::string> ApiClient::fetchLatestTree() {
    httplib::Client cli(domain);
    auto res = cli.Get("/projects/" + project + "/plates/latest");
    if (!res || res->status != 200) {
        std::cout << "fetchLatestTree failed\n";
        return {};
    }
    auto j = json::parse(res->body)["tree"];
    std::map<std::string, std::string> result;
    for (auto& [path, hash] : j.items())
        result[path] = hash.get<std::string>();
    return result;
}

std::string ApiClient::downloadBlob(const std::string& hash) {
    if (store.exists(hash)) return store.read(hash);
    httplib::Client cli(domain);
    auto res = cli.Get("/projects/" + project + "/blobs/" + hash);
    if (!res || res->status != 200) {
        std::cout << "downloadBlob failed: " << hash << "\n";
        return "";
    }
    store.write(hash, res->body);
    return res->body;
}

std::map<std::string, std::string> ApiClient::fetchTree(const std::string& plateId) {
    httplib::Client cli(domain);
    auto res = cli.Get("/projects/" + project + "/plates/" + plateId + "/tree");
    if (!res || res->status != 200) {
        std::cout << "fetchTree failed\n";
        return {};
    }
    auto j = json::parse(res->body);
    std::map<std::string, std::string> result;
    for (auto& [path, hash] : j.items())
        result[path] = hash.get<std::string>();
    return result;
}

std::vector<PlateInfo> ApiClient::fetchLog() {
    httplib::Client cli(domain);

    auto allRes = cli.Get("/projects/" + project + "/plates");
    if (!allRes || allRes->status != 200) { std::cout << "fetchLog failed\n"; return {}; }

    std::map<std::string, PlateInfo> byId;
    for (const auto& p : json::parse(allRes->body)) {
        PlateInfo pi;
        pi.id        = p.value("id",     "");
        pi.parent    = p.value("parent", "");
        pi.name      = p.value("name",   "");
        pi.flag      = p.value("flag",   "Normal");
        pi.fileCount = p.contains("tree") ? (int)p["tree"].size() : 0;
        byId[pi.id]  = pi;
    }

    auto headRes = cli.Get("/projects/" + project);
    if (!headRes || headRes->status != 200) return {};
    std::string head = json::parse(headRes->body).value("latest_plate", "");

    std::vector<PlateInfo> chain;
    std::set<std::string> seen;
    std::string cur = head;
    while (!cur.empty() && byId.count(cur) && !seen.count(cur)) {
        seen.insert(cur);
        chain.push_back(byId[cur]);
        cur = byId[cur].parent;
    }

    // plates with broken/old parent links not reached by chain walk
    for (const auto& [id, pi] : byId)
        if (!seen.count(id)) chain.push_back(pi);

    return chain;
}

std::vector<std::string> ApiClient::listProjects() {
    httplib::Client cli(domain);
    auto res = cli.Get("/projects");
    if (!res || res->status != 200) { std::cout << "listProjects failed\n"; return {}; }
    auto j = json::parse(res->body);
    std::vector<std::string> result;
    for (const auto& p : j) result.push_back(p.get<std::string>());
    return result;
}

bool ApiClient::initProject(const std::string& name) {
    httplib::Client cli(domain);
    json body = {{"name", name}};
    auto res = cli.Post("/projects", body.dump(), "application/json");
    if (!res) { std::cout << "initProject: server unreachable\n"; return false; }
    if (res->status == 409) { std::cout << "Project '" << name << "' already exists on server\n"; return true; }
    if (res->status != 201) { std::cout << "initProject failed: " << res->status << "\n"; return false; }
    return true;
}

bool ApiClient::deleteProject() {
    httplib::Client cli(domain);
    auto res = cli.Delete("/projects/" + project);
    return res && res->status == 200;
}
