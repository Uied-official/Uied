#include "uied.h"
#include <iostream>

int termID;

void handleCommand(std::string cmd) {
    if (cmd == "help") {
        uied::Engine::getInstance().print(termID, "Available: help, clear, hello, exit, about");
    }
    else if (cmd == "clear") {
        uied::Engine::getInstance().clear(termID);
    }
    else if (cmd == "hello") {
        uied::Engine::getInstance().print(termID, "System: Welcome to UIED Terminal, User!");
    }
    else if (cmd == "about") {
        uied::Engine::getInstance().print(termID, "UIED Engine v3.5 - Terminal Edition");
        uied::Engine::getInstance().print(termID, "Status: System Healthy.");
    }
    else if (cmd == "exit") {
        exit(0);
    }
    else if (cmd != "") {
        uied::Engine::getInstance().print(termID, "Error: Unknown command '" + cmd + "'");
    }
}

int main() {
    termID = uied::terminal(10, 10, 460, 340, handleCommand);

    uied::Engine::getInstance().print(termID, "--- UIED TERMINAL BOOTING... ---");
    uied::Engine::getInstance().print(termID, "Type 'help' to see commands.");

    uied::init(500, 400, "UIED System Terminal");

    return 0;
}