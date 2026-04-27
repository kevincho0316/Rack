#include "httplib.h"
#include <cstdlib>
#include <iostream>
#include "Storage.h"
#include "Routes.h"
#include "Color.h"

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::ios::sync_with_stdio(true);

    const char* envKey = std::getenv("RACK_API_KEY");
    std::string serverKey = envKey ? envKey : "";
    if (!serverKey.empty())
        std::cout << Color::y("[AUTH]") << " API key auth enabled\n";

    ServerStorage storage("data");
    storage.init();

    httplib::Server svr;
    registerRoutes(svr, storage, serverKey);

    svr.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::string method = req.method;
        std::string colored_method;
        if      (method == "GET")    colored_method = Color::c(method);
        else if (method == "POST")   colored_method = Color::g(method);
        else if (method == "DELETE") colored_method = Color::r(method);
        else if (method == "PATCH")  colored_method = Color::y(method);
        else                         colored_method = method;

        int s = res.status;
        std::string ss = std::to_string(s);
        std::string colored_status = s >= 500 ? Color::rb(ss)
                                   : s >= 400 ? Color::y(ss)
                                   : s >= 200 ? Color::g(ss)
                                   : ss;

        std::cout << colored_method << " " << req.path << " -> " << colored_status << std::endl;
    });

    std::cout << Color::gb("Server:") << " http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
