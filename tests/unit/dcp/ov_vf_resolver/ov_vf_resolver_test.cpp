#include "asset_map.hpp"
#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"
#include "packing_list.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using dcplayer::dcp::assetmap::AssetEntry;
using dcplayer::dcp::assetmap::AssetMap;
using dcplayer::dcp::assetmap::ChunkEntry;
using dcplayer::dcp::cpl::AssetReference;
using dcplayer::dcp::cpl::CompositionPlaylist;
using dcplayer::dcp::cpl::DiagnosticSeverity;
using dcplayer::dcp::cpl::Rational;
using dcplayer::dcp::cpl::Reel;
using dcplayer::dcp::cpl::TrackType;
using dcplayer::dcp::ov_vf_resolver::CompositionKind;
using dcplayer::dcp::ov_vf_resolver::DependencyKind;
using dcplayer::dcp::ov_vf_resolver::Package;
using dcplayer::dcp::ov_vf_resolver::ResolveRequest;
using dcplayer::dcp::ov_vf_resolver::ValidationStatus;
using dcplayer::dcp::pkl::AssetType;
using dcplayer::dcp::pkl::DigestAlgorithm;
using dcplayer::dcp::pkl::DigestEntry;
using dcplayer::dcp::pkl::PackingList;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

ChunkEntry make_chunk(std::string path) {
    return ChunkEntry{
        .path = std::move(path),
        .offset = 0U,
        .length = 1024U,
        .volume_index = 1U,
    };
}

AssetEntry make_asset_map_asset(std::string asset_id, std::string path) {
    return AssetEntry{
        .asset_id = std::move(asset_id),
        .chunk_list = {make_chunk(std::move(path))},
    };
}

dcplayer::dcp::pkl::AssetEntry make_pkl_asset(std::string asset_id, AssetType asset_type, std::string original_file_name) {
    return dcplayer::dcp::pkl::AssetEntry{
        .asset_id = std::move(asset_id),
        .asset_type = asset_type,
        .type_value = asset_type == AssetType::composition_playlist ? "text/xml;asdcpKind=CPL" : "application/mxf",
        .original_filename = std::move(original_file_name),
        .size_bytes = 1024U,
        .hash =
            DigestEntry{
                .algorithm = DigestAlgorithm::sha1,
                .value = "00112233445566778899aabbccddeeff00112233",
                .normalized_hex = std::string{"00112233445566778899aabbccddeeff00112233"},
            },
    };
}

AssetReference make_reference(std::string asset_id, TrackType track_type) {
    return AssetReference{
        .asset_id = std::move(asset_id),
        .track_type = track_type,
        .edit_rate =
            Rational{
                .numerator = 24U,
                .denominator = 1U,
            },
    };
}

CompositionPlaylist make_cpl(std::string composition_id,
                             std::string title,
                             std::vector<Reel> reels) {
    return CompositionPlaylist{
        .composition_id = std::move(composition_id),
        .content_title_text = std::move(title),
        .issuer = "dcplayer-tests",
        .issue_date_utc = "2026-03-11T08:30:00Z",
        .edit_rate =
            Rational{
                .numerator = 24U,
                .denominator = 1U,
            },
        .namespace_uri = "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#",
        .schema_flavor = dcplayer::dcp::cpl::SchemaFlavor::interop,
        .reels = std::move(reels),
    };
}

AssetMap make_asset_map(std::string asset_map_id, std::vector<AssetEntry> assets) {
    return AssetMap{
        .asset_map_id = std::move(asset_map_id),
        .issuer = "dcplayer-tests",
        .issue_date_utc = "2026-03-11T08:30:00Z",
        .creator = "dcplayer-tests",
        .volume_count = 1U,
        .namespace_uri = "http://www.digicine.com/PROTO-ASDCP-AM-20040311#",
        .schema_flavor = dcplayer::dcp::assetmap::SchemaFlavor::interop,
        .assets = std::move(assets),
    };
}

PackingList make_pkl(std::string pkl_id, std::vector<dcplayer::dcp::pkl::AssetEntry> assets) {
    return PackingList{
        .pkl_id = std::move(pkl_id),
        .issuer = "dcplayer-tests",
        .creator = "dcplayer-tests",
        .issue_date_utc = "2026-03-11T08:30:00Z",
        .namespace_uri = "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#",
        .schema_flavor = dcplayer::dcp::pkl::SchemaFlavor::interop,
        .assets = std::move(assets),
    };
}

Package make_package(AssetMap asset_map, PackingList packing_list, std::vector<CompositionPlaylist> compositions) {
    return Package{
        .asset_map = std::move(asset_map),
        .packing_list = std::move(packing_list),
        .composition_playlists = std::move(compositions),
    };
}

void expect_single_diagnostic(const dcplayer::dcp::ov_vf_resolver::ResolveResult& result,
                              const std::string& code,
                              const std::string& path) {
    expect(result.diagnostics.size() == 1U, "Expected exactly one diagnostic");
    expect(result.diagnostics[0].code == code, "Unexpected diagnostic code");
    expect(result.diagnostics[0].path == path, "Unexpected diagnostic path");
}

}  // namespace

int main() {
    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000001";
        const std::string picture_id = "32000000-0000-0000-0000-000000000001";
        const std::string sound_id = "33000000-0000-0000-0000-000000000001";

        auto package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000001",
                           {
                               make_asset_map_asset(composition_id, "CPL/ov.xml"),
                               make_asset_map_asset(picture_id, "MXF/ov_picture.mxf"),
                               make_asset_map_asset(sound_id, "MXF/ov_sound.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000001",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/ov.xml"),
                         make_pkl_asset(picture_id, AssetType::track_file, "MXF/ov_picture.mxf"),
                         make_pkl_asset(sound_id, AssetType::track_file, "MXF/ov_sound.mxf"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_OV",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000001",
                                 .picture = make_reference(picture_id, TrackType::picture),
                                 .sound = make_reference(sound_id, TrackType::sound),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(package)},
        });

        expect(result.ok(), "OV package should resolve");
        expect(result.validation_status == ValidationStatus::ok, "Expected clean validation status");
        expect(result.composition_graph.has_value(), "Expected composition graph");
        expect(result.composition_graph->composition_kind == CompositionKind::ov, "Expected OV composition kind");
        expect(result.composition_graph->source_packages.size() == 1U, "Expected one source package");
        expect(result.composition_graph->resolved_reels.size() == 1U, "Expected one resolved reel");
        expect(result.composition_graph->resolved_reels[0].picture_track.has_value(), "Expected picture track");
        expect(result.composition_graph->resolved_reels[0].picture_track->dependency_kind == DependencyKind::local,
               "Expected local picture dependency");
        expect(result.composition_graph->resolved_reels[0].sound_track->resolved_path == "MXF/ov_sound.mxf",
               "Unexpected sound path");
    }

    {
        const std::string ov_sound_id = "33000000-0000-0000-0000-000000000002";
        const std::string vf_composition_id = "30000000-0000-0000-0000-000000000003";
        const std::string vf_picture_id = "32000000-0000-0000-0000-000000000003";

        auto ov_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000002",
                           {
                               make_asset_map_asset(ov_sound_id, "MXF/ov_sound.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000002",
                     {
                         make_pkl_asset(ov_sound_id, AssetType::track_file, "MXF/ov_sound.mxf"),
                     }),
            {});

        auto vf_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000003",
                           {
                               make_asset_map_asset(vf_composition_id, "CPL/vf.xml"),
                               make_asset_map_asset(vf_picture_id, "MXF/vf_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000003",
                     {
                         make_pkl_asset(vf_composition_id, AssetType::composition_playlist, "CPL/vf.xml"),
                         make_pkl_asset(vf_picture_id, AssetType::track_file, "MXF/vf_picture.mxf"),
                     }),
            {
                make_cpl(vf_composition_id,
                         "Feature_VF",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000003",
                                 .picture = make_reference(vf_picture_id, TrackType::picture),
                                 .sound = make_reference(ov_sound_id, TrackType::sound),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = vf_composition_id,
            .packages = {std::move(ov_package), std::move(vf_package)},
        });

        expect(result.ok(), "VF package should resolve");
        expect(result.composition_graph.has_value(), "Expected VF composition graph");
        expect(result.composition_graph->composition_kind == CompositionKind::vf, "Expected VF composition kind");
        expect(result.composition_graph->source_packages.size() == 2U, "Expected OV and VF source packages");
        expect(result.composition_graph->resolved_reels[0].sound_track.has_value(), "Expected sound track");
        expect(result.composition_graph->resolved_reels[0].sound_track->dependency_kind == DependencyKind::external,
               "Expected external sound dependency");
        expect(result.composition_graph->resolved_reels[0].sound_track->source_package_id ==
                   "20000000-0000-0000-0000-000000000002",
               "Unexpected external source package");
    }

    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000004";
        const std::string missing_picture_id = "32000000-0000-0000-0000-000000000004";

        auto package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000004",
                           {
                               make_asset_map_asset(composition_id, "CPL/missing.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000004",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/missing.xml"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_Missing",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000004",
                                 .picture = make_reference(missing_picture_id, TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(package)},
        });

        expect(!result.ok(), "Missing asset must fail");
        expect(result.validation_status == ValidationStatus::error, "Expected error validation status");
        expect(!result.composition_graph.has_value(), "Graph should be absent on error");
        expect(result.diagnostics.size() == 2U, "Expected missing asset and broken dependency diagnostics");
        expect(result.diagnostics[0].code == "ov_vf.broken_dependency", "Expected broken dependency diagnostic");
        expect(result.diagnostics[1].code == "ov_vf.missing_asset_id", "Expected missing asset diagnostic");
        expect(result.diagnostics[1].path == "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id",
               "Unexpected missing asset path");
    }

    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000005";
        const std::string picture_id = "32000000-0000-0000-0000-000000000005";

        auto package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000005",
                           {
                               make_asset_map_asset(composition_id, "CPL/broken.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000005",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/broken.xml"),
                         make_pkl_asset(picture_id, AssetType::track_file, "MXF/broken_picture.mxf"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_Broken",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000005",
                                 .picture = make_reference(picture_id, TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(package)},
        });

        expect(!result.ok(), "Broken local reference must fail");
        expect_single_diagnostic(result,
                                 "ov_vf.broken_reference",
                                 "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id");
    }

    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000006";
        const std::string picture_id = "32000000-0000-0000-0000-000000000006";

        auto dep_a = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000007",
                           {
                               make_asset_map_asset(picture_id, "MXF/dep_a_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000007",
                     {
                         make_pkl_asset(picture_id, AssetType::track_file, "MXF/dep_a_picture.mxf"),
                     }),
            {});
        auto dep_b = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000008",
                           {
                               make_asset_map_asset(picture_id, "MXF/dep_b_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000008",
                     {
                         make_pkl_asset(picture_id, AssetType::track_file, "MXF/dep_b_picture.mxf"),
                     }),
            {});
        auto vf_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000006",
                           {
                               make_asset_map_asset(composition_id, "CPL/conflict.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000006",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/conflict.xml"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_Conflict",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000006",
                                 .picture = make_reference(picture_id, TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(vf_package), std::move(dep_b), std::move(dep_a)},
        });

        expect(!result.ok(), "Conflicting external candidates must fail");
        expect(result.diagnostics.size() == 2U, "Expected conflict and broken dependency diagnostics");
        expect(result.diagnostics[0].code == "ov_vf.broken_dependency", "Expected broken dependency");
        expect(result.diagnostics[1].code == "ov_vf.conflicting_asset_resolution",
               "Expected conflicting asset resolution");
    }

    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000007";
        const std::string picture_id = "32000000-0000-0000-0000-000000000007";

        auto stray_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000009", {}),
            make_pkl("20000000-0000-0000-0000-000000000009", {}),
            {
                make_cpl(composition_id,
                         "Feature_Stray",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000007",
                                 .picture = make_reference(picture_id, TrackType::picture),
                             },
                         }),
            });
        auto owner_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000010",
                           {
                               make_asset_map_asset(composition_id, "CPL/owner.xml"),
                               make_asset_map_asset(picture_id, "MXF/owner_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000010",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/owner.xml"),
                         make_pkl_asset(picture_id, AssetType::track_file, "MXF/owner_picture.mxf"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_Owner",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000008",
                                 .picture = make_reference(picture_id, TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(stray_package), std::move(owner_package)},
        });

        expect(result.ok(), "Stray CPL must not shadow real backed owner");
        expect(result.composition_graph.has_value(), "Expected graph for backed owner");
        expect(result.composition_graph->origin_package_id == "20000000-0000-0000-0000-000000000010",
               "Unexpected owner package");
        expect(result.composition_graph->resolved_reels[0].picture_track->resolved_path == "MXF/owner_picture.mxf",
               "Unexpected owner-backed picture path");
    }

    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000008";
        const std::string picture_a_id = "32000000-0000-0000-0000-000000000008";
        const std::string picture_b_id = "32000000-0000-0000-0000-000000000009";

        auto owner_a = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000011",
                           {
                               make_asset_map_asset(composition_id, "CPL/owner_a.xml"),
                               make_asset_map_asset(picture_a_id, "MXF/owner_a_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000011",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/owner_a.xml"),
                         make_pkl_asset(picture_a_id, AssetType::track_file, "MXF/owner_a_picture.mxf"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_Owner_A",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000009",
                                 .picture = make_reference(picture_a_id, TrackType::picture),
                             },
                         }),
            });
        auto owner_b = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000012",
                           {
                               make_asset_map_asset(composition_id, "CPL/owner_b.xml"),
                               make_asset_map_asset(picture_b_id, "MXF/owner_b_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000012",
                     {
                         make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/owner_b.xml"),
                         make_pkl_asset(picture_b_id, AssetType::track_file, "MXF/owner_b_picture.mxf"),
                     }),
            {
                make_cpl(composition_id,
                         "Feature_Owner_B",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000010",
                                 .picture = make_reference(picture_b_id, TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(owner_a), std::move(owner_b)},
        });

        expect(!result.ok(), "Two backed owners must conflict");
        expect_single_diagnostic(result, "ov_vf.conflicting_composition_owner", "/CompositionPlaylist/Id");
    }

    {
        const std::string composition_id = "30000000-0000-0000-0000-000000000009";
        const std::string picture_id = "32000000-0000-0000-0000-000000000010";

        auto stray_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000013", {}),
            make_pkl("20000000-0000-0000-0000-000000000013", {}),
            {
                make_cpl(composition_id,
                         "Feature_Stray_Only",
                         {
                             Reel{
                                 .reel_id = "31000000-0000-0000-0000-000000000011",
                                 .picture = make_reference(picture_id, TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
            .composition_id = composition_id,
            .packages = {std::move(stray_package)},
        });

        expect(!result.ok(), "Parsed-only unbacked CPL must not resolve");
        expect_single_diagnostic(result, "ov_vf.target_composition_not_found", "/CompositionPlaylist/Id");
    }

    return 0;
}
