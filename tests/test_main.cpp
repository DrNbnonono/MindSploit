#include <iostream>
#include "core/command.h"
#include "ai/ai_manager.h"
#include "modules/nmap_module.h"
#include "utils/process.h"

int main() {
    CommandSystem cmd;
    AIManager ai;
    NmapModule nmap;
    std::cout << cmd.runHello() << std::endl;
    std::cout << ai.hello() << std::endl;
    std::cout << nmap.hello() << std::endl;
    std::cout << ProcessUtils::hello() << std::endl;
    return 0;
} 