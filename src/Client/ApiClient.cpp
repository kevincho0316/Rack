#include "ApiClient.h"
#include <iostream>
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
                                   const std::string& name) {
    httplib::Client cli(domain);
    json treeJson = json::object();
    for (const auto& [path, hash] : tree) treeJson[path] = hash;
    json body = {{"parent", parent}, {"name", name}, {"flag", "Normal"}, {"tree", treeJson}};
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

bool ApiClient::initProject(const std::string& name) {
    httplib::Client cli(domain);
    json body = {{"name", name}};
    auto res = cli.Post("/projects", body.dump(), "application/json");
    if (!res) { std::cout << "initProject: server unreachable\n"; return false; }
    if (res->status == 409) { std::cout << "Project '" << name << "' already exists on server\n"; return true; }
    if (res->status != 201) { std::cout << "initProject failed: " << res->status << "\n"; return false; }
    return true;
}
