#pragma once

#include <string>
#include <string_view>

namespace dcplayer::core {

[[nodiscard]] std::string_view project_name() noexcept;
[[nodiscard]] std::string_view scaffold_version() noexcept;
[[nodiscard]] std::string make_banner(std::string_view component);

}  // namespace dcplayer::core
