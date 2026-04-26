#include <iostream>
#include "Rack.h"
#include "Commands.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: rack <command>\n"
                     "  commit                  hash & store current files, push if server up\n"
                     "  push                    upload local HEAD to server\n"
                     "  pull [-o]               sync local files to server HEAD (-o: overwrite only)\n"
                     "  log                     show plate history (HEAD → root)\n"
                     "  files                   list files in latest server plate\n"
                     "  status                  compare local HEAD vs server HEAD\n"
                     "  restore <plate-id>      restore files from a specific plate\n"
                     "  init <project>          create/activate project on server\n"
                     "  delete-project          delete active project from server (with confirmation)\n"
                     "  domain <url>            set server URL (saved globally)\n"
                     "  cat <hash>              print object by hash\n"
                     "  ls                      list all object hashes\n"
                     "  reconstruct             rebuild files from local HEAD\n"
                     "  serverCheck             exit 0 if server reachable\n";
        return 1;
    }

    Rack rack;
    std::string cmd = argv[1];

    if      (cmd == "commit")              return Commands::commit(rack);
    else if (cmd == "push")                return Commands::push(rack);
    else if (cmd == "pull") {
        bool overwriteOnly = (argc == 3 && std::string(argv[2]) == "-o");
        return Commands::pull(rack, overwriteOnly);
    }
    else if (cmd == "init"  && argc == 3)  return Commands::init(rack, argv[2]);
    else if (cmd == "cat"   && argc == 3)  return Commands::cat(rack, argv[2]);
    else if (cmd == "ls")                  return Commands::ls(rack);
    else if (cmd == "domain"   && argc == 3)  return Commands::setDomain(rack, argv[2]);
    else if (cmd == "reconstruct")            return Commands::reconstruct(rack);
    else if (cmd == "serverCheck")            return Commands::serverCheck(rack);
    else if (cmd == "log")                    return Commands::log(rack);
    else if (cmd == "files")                  return Commands::files(rack);
    else if (cmd == "status")                 return Commands::status(rack);
    else if (cmd == "restore"  && argc == 3)  return Commands::restore(rack, argv[2]);
    else if (cmd == "delete-project")         return Commands::deleteProject(rack);
    else { std::cout << "Unknown command: " << cmd << "\n"; return 1; }
}
