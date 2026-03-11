#include "version.hpp"

#include <string>

int main() {
    if (dcplayer::core::project_name() != std::string_view{"dcplayer"}) {
        return 1;
    }
    if (dcplayer::core::scaffold_version().find("scaffold") == std::string_view::npos) {
        return 2;
    }
    if (dcplayer::core::make_banner("core").empty()) {
        return 3;
    }
    return 0;
}
