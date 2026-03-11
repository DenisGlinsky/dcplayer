#include "version.hpp"

#include <string>

namespace dcplayer::core {

std::string_view project_name() noexcept {
    return "dcplayer";
}

std::string_view scaffold_version() noexcept {
    return "0.2.0-scaffold";
}

std::string make_banner(std::string_view component) {
    std::string banner{project_name()};
    banner += " ";
    banner += std::string{scaffold_version()};
    banner += " :: ";
    banner += std::string{component};
    banner += " (branch-sized scaffold)";
    return banner;
}

}  // namespace dcplayer::core
