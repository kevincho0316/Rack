#include <iostream>
#include "Rack.h"
#include "Commands.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: rack <commit|push|pull|init <project>|cat <hash>|ls"
                     "|domain <url>|reconstruct|serverCheck>\n";
        return 1;
    }

    Rack rack;
    std::string cmd = argv[1];

    if      (cmd == "commit")              return Commands::commit(rack);
    else if (cmd == "push")                return Commands::push(rack);
    else if (cmd == "pull")                return Commands::pull(rack);
    else if (cmd == "init"  && argc == 3)  return Commands::init(rack, argv[2]);
    else if (cmd == "cat"   && argc == 3)  return Commands::cat(rack, argv[2]);
    else if (cmd == "ls")                  return Commands::ls(rack);
    else if (cmd == "domain"&& argc == 3)  return Commands::setDomain(rack, argv[2]);
    else if (cmd == "reconstruct")         return Commands::reconstruct(rack);
    else if (cmd == "serverCheck")         return Commands::serverCheck(rack);
    else { std::cout << "Unknown command: " << cmd << "\n"; return 1; }
}
