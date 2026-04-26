#include "Routes.h"
#include "hash.h"
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static bool requireProject(ServerStorage& s, const std::string& p, httplib::Response& res) {
    if (!s.projectExists(p)) {
        res.status = 404;
        res.set_content(R"({"error":"project not found"})", "application/json");
        return false;
    }
    return true;
}

void registerRoutes(httplib::Server& svr, ServerStorage& storage) {

    svr.Get("/hello", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });

    // ---- projects ----

    svr.Get("/projects", [&storage](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        for (const auto& p : storage.listProjects()) arr.push_back(p);
        res.set_content(arr.dump(), "application/json");
    });

    svr.Post("/projects", [&storage](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::string name = j.at("name");
            if (storage.projectExists(name)) {
                res.status = 409;
                res.set_content(R"({"error":"project exists"})", "application/json");
                return;
            }
            storage.createProject(name);
            std::cout << "  [project] created: " << name << std::endl;
            res.status = 201;
            res.set_content(json{{"name", name}}.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"bad request"})", "application/json");
        }
    });

    // ---- blobs ----

    svr.Post(R"(/projects/([^/]+)/blobs/check)",
             [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        try {
            auto j = json::parse(req.body);
            json missing = json::array();
            auto hashes = j.at("hashes");
            for (const auto& h : hashes) {
                if (!storage.blobExists(p, h.get<std::string>()))
                    missing.push_back(h);
            }
            std::cout << "  [check] project=" << p
                      << " requested=" << hashes.size()
                      << " missing=" << missing.size() << std::endl;
            res.set_content(json{{"missing", missing}}.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"bad request"})", "application/json");
        }
    });

    svr.Post(R"(/projects/([^/]+)/blobs)",
             [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        std::string hash = GetHash(req.body);
        bool existed = storage.blobExists(p, hash);
        if (!existed) storage.writeBlob(p, hash, req.body);
        std::cout << "  [blob] " << (existed ? "exists" : "stored")
                  << " " << hash.substr(0, 12) << " (" << req.body.size() << " bytes) project=" << p << std::endl;
        res.status = 201;
        res.set_content(json{{"hash", hash}}.dump(), "application/json");
    });

    svr.Get(R"(/projects/([^/]+)/blobs/([a-f0-9]+))",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p    = req.matches[1];
        std::string hash = req.matches[2];
        if (!requireProject(storage, p, res)) return;
        if (!storage.blobExists(p, hash)) {
            std::cout << "  [blob] miss " << hash.substr(0, 12) << " project=" << p << std::endl;
            res.status = 404; return;
        }
        auto data = storage.readBlob(p, hash);
        std::cout << "  [blob] sent " << hash.substr(0, 12) << " (" << data.size() << " bytes) project=" << p << std::endl;
        res.set_content(data, "application/octet-stream");
    });

    // ---- plates ----

    svr.Post(R"(/projects/([^/]+)/plates)",
             [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        try {
            auto j = json::parse(req.body);
            std::string parent = j.value("parent", "");
            std::string name   = j.value("name", "");
            std::string note   = j.value("note", "");
            std::string flag   = j.value("flag", "Normal");
            json tree          = j.at("tree");

            // ID = hash of immutable parts only (parent + sorted tree)
            json id_src = {{"parent", parent}, {"tree", tree}};
            std::string id = GetHash(id_src.dump());

            auto uploaded_at = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            json plate = {{"id", id}, {"parent", parent}, {"name", name},
                          {"note", note}, {"flag", flag}, {"tree", tree},
                          {"uploaded_at", uploaded_at}};

            bool existed = storage.plateExists(p, id);
            if (!existed) {
                storage.writePlate(p, id, plate.dump());
                storage.writeHead(p, id);
            }
            std::cout << "  [plate] " << (existed ? "exists" : "created")
                      << " id=" << id.substr(0, 12)
                      << " parent=" << (parent.empty() ? "(none)" : parent.substr(0, 12))
                      << " name=\"" << name << "\""
                      << " files=" << tree.size()
                      << " project=" << p << std::endl;
            res.status = 201;
            res.set_content(plate.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"bad request"})", "application/json");
        }
    });

    svr.Get(R"(/projects/([^/]+)/plates/latest)",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        std::string head = storage.readHead(p);
        if (head.empty()) {
            res.status = 404;
            res.set_content(R"({"error":"no plates"})", "application/json");
            return;
        }
        res.set_content(storage.readPlate(p, head), "application/json");
    });

    svr.Patch(R"(/projects/([^/]+)/plates/latest)",
              [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        std::string head = storage.readHead(p);
        if (head.empty()) { res.status = 404; return; }
        try {
            auto j  = json::parse(req.body);
            auto pl = json::parse(storage.readPlate(p, head));
            if (j.contains("flag")) pl["flag"] = j["flag"];
            if (j.contains("note")) pl["note"] = j["note"];
            storage.writePlate(p, head, pl.dump());
            res.set_content(pl.dump(), "application/json");
        } catch (...) {
            res.status = 400;
        }
    });

    svr.Get(R"(/projects/([^/]+)/plates/([a-f0-9]+)/tree)",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p  = req.matches[1];
        std::string id = req.matches[2];
        if (!requireProject(storage, p, res)) return;
        if (!storage.plateExists(p, id)) { res.status = 404; return; }
        auto pl = json::parse(storage.readPlate(p, id));
        res.set_content(pl["tree"].dump(), "application/json");
    });

    svr.Get(R"(/projects/([^/]+)/plates/([a-f0-9]+))",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p  = req.matches[1];
        std::string id = req.matches[2];
        if (!requireProject(storage, p, res)) return;
        if (!storage.plateExists(p, id)) { res.status = 404; return; }
        res.set_content(storage.readPlate(p, id), "application/json");
    });

    svr.Get(R"(/projects/([^/]+)/plates)",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        std::string flag_filter = req.has_param("flag") ? req.get_param_value("flag") : "";
        json arr = json::array();
        for (const auto& id : storage.listPlateIds(p)) {
            auto data = storage.readPlate(p, id);
            if (data.empty()) continue;
            try {
                auto pl = json::parse(data);
                if (!flag_filter.empty() && pl.value("flag", "") != flag_filter) continue;
                arr.push_back(pl);
            } catch (...) {}
        }
        res.set_content(arr.dump(), "application/json");
    });

    // ---- diff ----

    svr.Get(R"(/projects/([^/]+)/diff)",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        if (!req.has_param("from") || !req.has_param("to")) {
            res.status = 400;
            res.set_content(R"({"error":"from and to required"})", "application/json");
            return;
        }
        std::string from_id = req.get_param_value("from");
        std::string to_id   = req.get_param_value("to");
        if (!storage.plateExists(p, from_id) || !storage.plateExists(p, to_id)) {
            res.status = 404;
            res.set_content(R"({"error":"plate not found"})", "application/json");
            return;
        }
        try {
            json from_tree = json::parse(storage.readPlate(p, from_id))["tree"];
            json to_tree   = json::parse(storage.readPlate(p, to_id))["tree"];

            json added = json::object(), modified = json::object(), removed = json::array();
            for (auto& [path, hash] : to_tree.items()) {
                if (!from_tree.contains(path)) added[path] = hash;
                else if (from_tree[path] != hash) modified[path] = hash;
            }
            for (auto& [path, hash] : from_tree.items()) {
                if (!to_tree.contains(path)) removed.push_back(path);
            }
            res.set_content(
                json{{"added", added}, {"modified", modified}, {"removed", removed}}.dump(),
                "application/json");
        } catch (...) {
            res.status = 500;
        }
    });

    // ---- delete project ----

    svr.Delete(R"(/projects/([^/]+))",
               [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        storage.deleteProject(p);
        std::cout << "  [project] deleted: " << p << std::endl;
        res.set_content(json{{"deleted", p}}.dump(), "application/json");
    });

    // ---- project detail (last to avoid shadowing sub-routes) ----

    svr.Get(R"(/projects/([^/]+))",
            [&storage](const httplib::Request& req, httplib::Response& res) {
        std::string p = req.matches[1];
        if (!requireProject(storage, p, res)) return;
        std::string head = storage.readHead(p);
        res.set_content(json{{"name", p}, {"latest_plate", head}}.dump(), "application/json");
    });

    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"error":"not found"})", "application/json");
    });
}
