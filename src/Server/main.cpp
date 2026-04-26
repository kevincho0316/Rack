#include "httplib.h"
#include <iostream>
#include "Storage.h"
#include "Routes.h"

int main() {
    ServerStorage storage("data");
    storage.init();

    httplib::Server svr;
    registerRoutes(svr, storage);

    std::cout << "Server: http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
