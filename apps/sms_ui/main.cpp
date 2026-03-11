#include "version.hpp"

#include <iostream>
#include <string_view>

namespace {
int print_help() {
    std::cout << dcplayer::core::make_banner("sms_ui") << "\n";
    std::cout << "Usage: sms_ui [--help] [--version]\n";
    std::cout << "Future task owner: T09a.\n";
    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        return print_help();
    }
    const std::string_view arg{argv[1]};
    if (arg == "-h" || arg == "--help") {
        return print_help();
    }
    if (arg == "--version") {
        std::cout << dcplayer::core::scaffold_version() << "\n";
        return 0;
    }
    std::cerr << "Scaffold-only build: local SMS is not implemented yet.\n";
    return 2;
}
