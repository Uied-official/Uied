#include "uied.h"
#include <iostream>
#include <string>

int main() {
    uied::label("UIED", 20, 10, RGB(0, 255, 150));

    int termID = uied::terminal(20, 40, 440, 250, [=](std::string cmd) {
        if (cmd == "help") {
            uied::Engine::getInstance().print(termID, "Available: clear, hello, color, exit");
        }
        else if (cmd == "clear") {
            uied::Engine::getInstance().clear(termID);
        }
        else if (cmd == "hello") {
            uied::Engine::getInstance().print(termID, "System: Welcome, User!");
        }
        else if (cmd == "exit") {
            exit(0);
        }
        else if (!cmd.empty()) {
            uied::Engine::getInstance().print(termID, "Unknown command: " + cmd);
        }
    });

    uied::Engine::getInstance().print(termID, "--- UIED System Terminal Booted ---");
    uied::Engine::getInstance().print(termID, "Type 'help' for commands.");

    uied::label("Fast Message:", 20, 305);
    int inputID = uied::input(20, 325, 200, 30, "Enter text...");

    uied::button("Send to Term", 230, 325, 120, 30, RGB(0, 120, 215), [=]() {
        std::string text = uied::Engine::getInstance().getInput(inputID);
        if (!text.empty()) {
            uied::Engine::getInstance().print(termID, "User: " + text);
        }
    });

    uied::label("Adjust Parameter (0-100%):", 20, 370);
    uied::slider(20, 395, 330, RGB(255, 150, 0), [=](float val) {
        static int lastVal = -1;
        int currentVal = (int)(val * 100);
        if (currentVal != lastVal) {
            lastVal = currentVal;
        }
    });

    uied::button("Clear Logs", 360, 325, 100, 30, RGB(180, 50, 50), [=]() {
        uied::Engine::getInstance().clear(termID);
        uied::Engine::getInstance().print(termID, "Logs cleared.");
    });

    uied::init(500, 480, "UIED Framework Showcase");

    return 0;
}