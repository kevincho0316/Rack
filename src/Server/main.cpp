#include "httplib.h"
#include <iostream>
#include "Storage.h"
#include "Routes.h"

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::ios::sync_with_stdio(true);

    ServerStorage storage("data");
    storage.init();

    httplib::Server svr;
    registerRoutes(svr, storage);

    svr.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << req.method << " " << req.path << " -> " << res.status << std::endl;
    });

    std::cout << "Server: http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
