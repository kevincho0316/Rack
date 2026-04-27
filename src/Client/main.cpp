#include <iostream>
#include "Rack.h"
#include "Commands.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: rack <command> [options]\n"
                     "  commit [-n <name>] [-f <flag>]       hash & store, push if server up\n"
                     "  push   [project]                     upload local HEAD to server\n"
                     "  pull   [project] [-o]                sync to server HEAD (-o: no delete)\n"
                     "  diff                                 local HEAD vs server HEAD\n"
                     "  diff   <plateId>                     local HEAD vs specific plate\n"
                     "  diff   <plateIdA> <plateIdB>         server plate A vs server plate B\n"
                     "  log    [project]                     show plate history\n"
                     "  files  [project]                     list files in latest plate\n"
                     "  status [project]                     compare local HEAD vs server\n"
                     "  restore <plate-id> [project]         restore files from a plate\n"
                     "  projects                             list all projects on server\n"
                     "  init <project>                       create/activate project on server\n"
                     "  delete-project                       delete active project (with confirmation)\n"
                     "  domain <url>                         set server URL (saved globally)\n"
                     "  cat <hash>                           print object by hash\n"
                     "  ls                                   list all local object hashes\n"
                     "  reconstruct                          rebuild files from local HEAD\n"
                     "  serverCheck                          exit 0 if server reachable\n"
                     "  checkout <url> <project>             set domain, init project, pull files\n"
                     "  auth <key>                           set API key (saved globally in ~/.rack/config)\n";
        return 1;
    }

    Rack rack;
    std::string cmd = argv[1];

    if (cmd == "commit") {
        std::string name, flag = "Normal";
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-n" && i + 1 < argc) name = argv[++i];
            else if (a == "-f" && i + 1 < argc) flag = argv[++i];
        }
        return Commands::commit(rack, name, flag);
    } else if (cmd == "push") {
        std::string proj = (argc >= 3) ? argv[2] : "";
        return Commands::push(rack, proj);
    } else if (cmd == "pull") {
        std::string proj; bool overwriteOnly = false;
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-o") overwriteOnly = true;
            else proj = a;
        }
        return Commands::pull(rack, overwriteOnly, proj);
    }
    else if (cmd == "log") {
        std::string proj = (argc >= 3) ? argv[2] : "";
        return Commands::log(rack, proj);
    }
    else if (cmd == "files") {
        std::string proj = (argc >= 3) ? argv[2] : "";
        return Commands::files(rack, proj);
    }
    else if (cmd == "status") {
        std::string proj = (argc >= 3) ? argv[2] : "";
        return Commands::status(rack, proj);
    }
    else if (cmd == "diff") {
        std::string plateA = (argc >= 3) ? argv[2] : "";
        std::string plateB = (argc >= 4) ? argv[3] : "";
        return Commands::diff(rack, plateA, plateB);
    }
    else if (cmd == "restore" && argc >= 3) {
        std::string proj = (argc >= 4) ? argv[3] : "";
        return Commands::restore(rack, argv[2], proj);
    }
    else if (cmd == "init"  && argc == 3)     return Commands::init(rack, argv[2]);
    else if (cmd == "cat"   && argc == 3)     return Commands::cat(rack, argv[2]);
    else if (cmd == "ls")                     return Commands::ls(rack);
    else if (cmd == "domain" && argc == 3)    return Commands::setDomain(rack, argv[2]);
    else if (cmd == "reconstruct")            return Commands::reconstruct(rack);
    else if (cmd == "serverCheck")            return Commands::serverCheck(rack);
    else if (cmd == "projects")               return Commands::projects(rack);
    else if (cmd == "delete-project") {
        std::string proj; bool autoConfirm = false;
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-y") autoConfirm = true;
            else proj = a;
        }
        return Commands::deleteProject(rack, proj, autoConfirm);
    }
    else if (cmd == "checkout" && argc == 4) return Commands::checkout(rack, argv[2], argv[3]);
    else if (cmd == "auth"     && argc == 3) return Commands::setApiKey(rack, argv[2]);
    else { std::cout << "Unknown command: " << cmd << "\n"; return 1; }
}
