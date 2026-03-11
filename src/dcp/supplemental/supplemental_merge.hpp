#pragma once

#include "ov_vf_resolver.hpp"

#include <optional>
#include <string>
#include <vector>

namespace dcplayer::dcp::supplemental {

using CompositionGraph = ov_vf_resolver::CompositionGraph;
using Diagnostic = ov_vf_resolver::Diagnostic;
using DiagnosticSeverity = ov_vf_resolver::DiagnosticSeverity;
using Package = ov_vf_resolver::Package;
using ValidationStatus = ov_vf_resolver::ValidationStatus;

enum class MergeMode {
    replace_lane,
};

struct MergePolicy {
    std::string policy_id;
    std::string base_composition_id;
    std::string supplemental_composition_id;
    MergeMode merge_mode{MergeMode::replace_lane};
    std::vector<cpl::TrackType> allowed_overrides;
};

struct ApplyRequest {
    CompositionGraph base_graph;
    std::vector<Package> packages;
    std::vector<MergePolicy> policies;
};

struct ApplyResult {
    std::optional<CompositionGraph> composition_graph;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ApplyResult apply(const ApplyRequest& request);
[[nodiscard]] const char* to_string(MergeMode merge_mode) noexcept;

}  // namespace dcplayer::dcp::supplemental
