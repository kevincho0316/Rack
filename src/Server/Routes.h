#pragma once
#include <httplib.h>
#include "Storage.h"

void registerRoutes(httplib::Server& svr, ServerStorage& storage, const std::string& apiKey = "");
