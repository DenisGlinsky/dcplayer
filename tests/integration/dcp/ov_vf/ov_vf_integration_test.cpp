#include "asset_map.hpp"
#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"
#include "packing_list.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using dcplayer::dcp::ov_vf_resolver::Package;
using dcplayer::dcp::ov_vf_resolver::ResolveRequest;
using dcplayer::dcp::ov_vf_resolver::ValidationStatus;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open fixture: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

Package load_package(const std::filesystem::path& package_dir) {
    const auto asset_map_result = dcplayer::dcp::assetmap::parse(read_file(package_dir / "ASSETMAP.xml"));
    expect(asset_map_result.ok(), "AssetMap fixture must parse");
    expect(asset_map_result.asset_map.has_value(), "Expected normalized AssetMap");

    const auto pkl_result = dcplayer::dcp::pkl::parse(read_file(package_dir / "PKL.xml"));
    expect(pkl_result.ok(), "PKL fixture must parse");
    expect(pkl_result.packing_list.has_value(), "Expected normalized PKL");

    std::vector<dcplayer::dcp::cpl::CompositionPlaylist> compositions;
    const auto cpl_path = package_dir / "CPL.xml";
    if (std::filesystem::exists(cpl_path)) {
        const auto cpl_result = dcplayer::dcp::cpl::parse(read_file(cpl_path));
        expect(cpl_result.ok(), "CPL fixture must parse");
        expect(cpl_result.composition_playlist.has_value(), "Expected normalized CPL");
        compositions.push_back(*cpl_result.composition_playlist);
    }

    return Package{
        .asset_map = *asset_map_result.asset_map,
        .packing_list = *pkl_result.packing_list,
        .composition_playlists = std::move(compositions),
    };
}

}  // namespace

int main() {
    const std::filesystem::path fixture_root{OV_VF_FIXTURE_DIR};

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000101",
            .packages = {
                load_package(fixture_root / "valid" / "ov_single" / "ov_pkg"),
            },
        });

        expect(result.ok(), "OV fixture should resolve");
        expect(result.validation_status == ValidationStatus::ok, "Expected ok status");
        expect(result.composition_graph.has_value(), "Expected graph for OV fixture");
        expect(result.composition_graph->composition_id == "30000000-0000-0000-0000-000000000101",
               "Unexpected OV composition_id");
        expect(result.composition_graph->source_packages == std::vector<std::string>{
                                                          "20000000-0000-0000-0000-000000000101"},
               "Unexpected OV source packages");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000103",
            .packages = {
                load_package(fixture_root / "valid" / "vf_with_ov" / "ov_pkg"),
                load_package(fixture_root / "valid" / "vf_with_ov" / "vf_pkg"),
            },
        });

        expect(result.ok(), "VF fixture should resolve");
        expect(result.composition_graph.has_value(), "Expected graph for VF fixture");
        expect(std::string{dcplayer::dcp::ov_vf_resolver::to_string(result.composition_graph->composition_kind)} == "vf",
               "Expected VF composition kind");
        expect(result.composition_graph->resolved_reels[0].sound_track.has_value(), "Expected resolved external sound");
        expect(result.composition_graph->resolved_reels[0].sound_track->resolved_path == "MXF/ov_sound.mxf",
               "Unexpected resolved OV sound path");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000104",
            .packages = {
                load_package(fixture_root / "invalid" / "missing_asset_id" / "broken_pkg"),
            },
        });

        expect(!result.ok(), "Missing asset fixture must fail");
        expect(result.diagnostics.size() == 2U, "Expected missing asset and broken dependency diagnostics");
        expect(result.diagnostics[1].code == "ov_vf.missing_asset_id", "Expected missing asset diagnostic");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000105",
            .packages = {
                load_package(fixture_root / "invalid" / "broken_reference" / "broken_pkg"),
            },
        });

        expect(!result.ok(), "Broken reference fixture must fail");
        expect(result.diagnostics.size() == 1U, "Expected one broken reference diagnostic");
        expect(result.diagnostics[0].code == "ov_vf.broken_reference", "Expected broken reference diagnostic");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000106",
            .packages = {
                load_package(fixture_root / "invalid" / "conflicting_resolution" / "vf_pkg"),
                load_package(fixture_root / "invalid" / "conflicting_resolution" / "dep_a"),
                load_package(fixture_root / "invalid" / "conflicting_resolution" / "dep_b"),
            },
        });

        expect(!result.ok(), "Conflicting resolution fixture must fail");
        expect(result.diagnostics.size() == 2U, "Expected conflict and broken dependency diagnostics");
        expect(result.diagnostics[1].code == "ov_vf.conflicting_asset_resolution",
               "Expected conflicting resolution diagnostic");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000109",
            .packages = {
                load_package(fixture_root / "invalid" / "broken_dependency" / "vf_pkg"),
                load_package(fixture_root / "invalid" / "broken_dependency" / "dep_pkg"),
            },
        });

        expect(!result.ok(), "Broken dependency fixture must fail");
        expect(result.diagnostics.size() == 2U, "Expected broken reference and broken dependency diagnostics");
        expect(result.diagnostics[0].code == "ov_vf.broken_dependency", "Expected broken dependency diagnostic");
        expect(result.diagnostics[1].code == "ov_vf.broken_reference", "Expected broken reference diagnostic");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000111",
            .packages = {
                load_package(fixture_root / "valid" / "stray_with_real_owner" / "stray_pkg"),
                load_package(fixture_root / "valid" / "stray_with_real_owner" / "owner_pkg"),
            },
        });

        expect(result.ok(), "Stray parsed CPL must not block real backed owner");
        expect(result.composition_graph.has_value(), "Expected graph for real owner");
        expect(result.composition_graph->origin_package_id == "20000000-0000-0000-0000-000000000111",
               "Unexpected real owner package");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000112",
            .packages = {
                load_package(fixture_root / "invalid" / "double_owner_conflict" / "owner_a"),
                load_package(fixture_root / "invalid" / "double_owner_conflict" / "owner_b"),
            },
        });

        expect(!result.ok(), "Two backed owners must conflict");
        expect(result.diagnostics.size() == 1U, "Expected one owner-conflict diagnostic");
        expect(result.diagnostics[0].code == "ov_vf.conflicting_composition_owner",
               "Expected conflicting owner diagnostic");
    }

    {
        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = "30000000-0000-0000-0000-000000000113",
            .packages = {
                load_package(fixture_root / "invalid" / "parsed_only_no_backing" / "stray_pkg"),
            },
        });

        expect(!result.ok(), "Parsed-only unbacked CPL must not resolve");
        expect(result.diagnostics.size() == 1U, "Expected one not-found diagnostic");
        expect(result.diagnostics[0].code == "ov_vf.target_composition_not_found",
               "Expected target composition not found diagnostic");
    }

    return 0;
}
