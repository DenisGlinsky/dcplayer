#include "asset_map.hpp"
#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"
#include "packing_list.hpp"
#include "supplemental_merge.hpp"

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
using dcplayer::dcp::cpl::Rational;
using dcplayer::dcp::cpl::Reel;
using dcplayer::dcp::cpl::TrackType;
using dcplayer::dcp::ov_vf_resolver::CompositionGraph;
using dcplayer::dcp::ov_vf_resolver::CompositionKind;
using dcplayer::dcp::ov_vf_resolver::Package;
using dcplayer::dcp::ov_vf_resolver::ResolveRequest;
using dcplayer::dcp::ov_vf_resolver::ValidationStatus;
using dcplayer::dcp::pkl::AssetType;
using dcplayer::dcp::pkl::DigestAlgorithm;
using dcplayer::dcp::pkl::DigestEntry;
using dcplayer::dcp::pkl::PackingList;
using dcplayer::dcp::supplemental::ApplyRequest;
using dcplayer::dcp::supplemental::MergeMode;
using dcplayer::dcp::supplemental::MergePolicy;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

ChunkEntry make_chunk(std::string path) {
    return ChunkEntry{
        .path = std::move(path),
        .offset = 0U,
        .length = 2048U,
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
        .size_bytes = 2048U,
        .hash =
            DigestEntry{
                .algorithm = DigestAlgorithm::sha256,
                .value = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff",
                .normalized_hex = std::string{"00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"},
            },
    };
}

AssetReference make_reference(std::string asset_id, TrackType track_type, Rational edit_rate = Rational{24U, 1U}) {
    return AssetReference{
        .asset_id = std::move(asset_id),
        .track_type = track_type,
        .edit_rate = edit_rate,
    };
}

CompositionPlaylist make_cpl(std::string composition_id, std::string title, std::vector<Reel> reels) {
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

Package make_base_package(const std::string& composition_id,
                          const std::string& reel_id,
                          const std::string& picture_id,
                          const std::string& sound_id) {
    return make_package(
        make_asset_map("10000000-0000-0000-0000-000000000201",
                       {
                           make_asset_map_asset(composition_id, "CPL/base.xml"),
                           make_asset_map_asset(picture_id, "MXF/base_picture.mxf"),
                           make_asset_map_asset(sound_id, "MXF/base_sound.mxf"),
                       }),
        make_pkl("20000000-0000-0000-0000-000000000201",
                 {
                     make_pkl_asset(composition_id, AssetType::composition_playlist, "CPL/base.xml"),
                     make_pkl_asset(picture_id, AssetType::track_file, "MXF/base_picture.mxf"),
                     make_pkl_asset(sound_id, AssetType::track_file, "MXF/base_sound.mxf"),
                 }),
        {
            make_cpl(composition_id,
                     "Feature_Base",
                     {
                         Reel{
                             .reel_id = reel_id,
                             .picture = make_reference(picture_id, TrackType::picture),
                             .sound = make_reference(sound_id, TrackType::sound),
                         },
                     }),
        });
}

CompositionGraph resolve_base_graph(const std::string& composition_id, std::vector<Package> packages) {
    const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
        .composition_id = composition_id,
        .packages = std::move(packages),
    });

    expect(result.ok(), "Base graph must resolve");
    expect(result.composition_graph.has_value(), "Expected resolved base graph");
    return *result.composition_graph;
}

void expect_single_diagnostic(const dcplayer::dcp::supplemental::ApplyResult& result,
                              const std::string& code,
                              const std::string& path) {
    expect(result.diagnostics.size() == 1U, "Expected exactly one diagnostic");
    expect(result.diagnostics[0].code == code, "Unexpected diagnostic code");
    expect(result.diagnostics[0].path == path, "Unexpected diagnostic path");
}

}  // namespace

int main() {
    const std::string base_composition_id = "30000000-0000-0000-0000-000000000201";
    const std::string base_reel_id = "31000000-0000-0000-0000-000000000201";
    const std::string base_picture_id = "32000000-0000-0000-0000-000000000201";
    const std::string base_sound_id = "33000000-0000-0000-0000-000000000201";

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000202",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000202", "CPL/supp.xml"),
                               make_asset_map_asset("32000000-0000-0000-0000-000000000202", "MXF/supp_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000202",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000202",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                         make_pkl_asset("32000000-0000-0000-0000-000000000202",
                                        AssetType::track_file,
                                        "MXF/supp_picture.mxf"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000202",
                         "Feature_Supp_Picture",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000202", TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "supp-picture",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000202",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture},
                    },
                },
        });

        expect(result.ok(), "Supplemental picture override should resolve");
        expect(result.validation_status == ValidationStatus::ok, "Expected clean supplemental validation");
        expect(result.composition_graph.has_value(), "Expected merged graph");
        expect(result.composition_graph->resolved_reels[0].picture_track->resolved_path == "MXF/supp_picture.mxf",
               "Expected picture override from supplemental package");
        expect(result.composition_graph->resolved_reels[0].sound_track->resolved_path == "MXF/base_sound.mxf",
               "Expected inherited base sound");
        expect(result.composition_graph->composition_kind == CompositionKind::vf,
               "Supplemental external package should mark graph as vf");
        expect(result.composition_graph->source_packages ==
                   std::vector<std::string>{
                       "20000000-0000-0000-0000-000000000201",
                       "20000000-0000-0000-0000-000000000202",
                   },
               "Unexpected source packages after merge");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000203",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000203", "CPL/supp.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000203",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000203",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000203",
                         "Feature_Supp_Base_Dep",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .sound = make_reference(base_sound_id, TrackType::sound),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "supp-base-dep",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000203",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {},
                    },
                },
        });

        expect(result.ok(), "Explicit base dependency should keep base graph show-ready");
        expect(result.composition_graph.has_value(), "Expected merged graph");
        expect(result.composition_graph->resolved_reels[0].sound_track->asset_id == base_sound_id,
               "Expected inherited base sound asset");
        expect(result.composition_graph->source_packages ==
                   std::vector<std::string>{
                       "20000000-0000-0000-0000-000000000201",
                   },
               "Explicit base dependency must not add a supplemental source package");
        expect(result.composition_graph->composition_kind == CompositionKind::ov,
               "No external override should keep graph as ov");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000209",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000209", "CPL/supp.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000209",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000209",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000209",
                         "Feature_Supp_Base_EditRate_Mismatch",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .sound = make_reference(base_sound_id,
                                                         TrackType::sound,
                                                         Rational{
                                                             .numerator = 25U,
                                                             .denominator = 1U,
                                                         }),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "supp-base-rate-mismatch",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000209",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {},
                    },
                },
        });

        expect(!result.ok(), "Explicit base dependency with mismatched edit_rate must fail");
        expect_single_diagnostic(result,
                                 "supplemental.base_edit_rate_mismatch",
                                 "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainSound/Id");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "missing-target",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000299",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Missing supplemental target must fail");
        expect_single_diagnostic(result,
                                 "supplemental.target_composition_not_found",
                                 "/SupplementalMergePolicyList/Policy[1]/SupplementalCompositionId");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000210",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000210", "CPL/supp.xml"),
                               make_asset_map_asset("32000000-0000-0000-0000-000000000210", "MXF/supp_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000210",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000210",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                         make_pkl_asset("32000000-0000-0000-0000-000000000210",
                                        AssetType::track_file,
                                        "MXF/supp_picture.mxf"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000210",
                         "Feature_Supp_Invalid_Mode",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000210", TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "invalid-mode",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000210",
                        .merge_mode = static_cast<MergeMode>(99),
                        .allowed_overrides = {TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Unsupported merge_mode must fail");
        expect_single_diagnostic(result,
                                 "supplemental.unsupported_merge_mode",
                                 "/SupplementalMergePolicyList/Policy[1]/MergeMode");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000204",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000204", "CPL/supp.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000204",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000204",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000204",
                         "Feature_Supp_Broken_Base",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000299", TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "broken-base",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000204",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Broken base dependency must fail");
        expect_single_diagnostic(result,
                                 "supplemental.broken_base_dependency",
                                 "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supp_a = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000205",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000205", "CPL/supp_a.xml"),
                               make_asset_map_asset("32000000-0000-0000-0000-000000000205", "MXF/supp_a_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000205",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000205",
                                        AssetType::composition_playlist,
                                        "CPL/supp_a.xml"),
                         make_pkl_asset("32000000-0000-0000-0000-000000000205",
                                        AssetType::track_file,
                                        "MXF/supp_a_picture.mxf"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000205",
                         "Feature_Supp_A",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000205", TrackType::picture),
                             },
                         }),
            });
        auto supp_b = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000206",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000206", "CPL/supp_b.xml"),
                               make_asset_map_asset("32000000-0000-0000-0000-000000000206", "MXF/supp_b_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000206",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000206",
                                        AssetType::composition_playlist,
                                        "CPL/supp_b.xml"),
                         make_pkl_asset("32000000-0000-0000-0000-000000000206",
                                        AssetType::track_file,
                                        "MXF/supp_b_picture.mxf"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000206",
                         "Feature_Supp_B",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000206", TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supp_b), std::move(supp_a)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "policy-b",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000206",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture},
                    },
                    MergePolicy{
                        .policy_id = "policy-a",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000205",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Conflicting overrides must fail");
        expect_single_diagnostic(result,
                                 "supplemental.conflicting_override",
                                 "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000207",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000207", "CPL/supp.xml"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000207",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000207",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                         make_pkl_asset("32000000-0000-0000-0000-000000000207",
                                        AssetType::track_file,
                                        "MXF/missing_picture.mxf"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000207",
                         "Feature_Supp_Broken_Backing",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000207", TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "broken-backing",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000207",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Supplemental asset without valid backing must fail");
        expect_single_diagnostic(result,
                                 "supplemental.asset_without_valid_backing",
                                 "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        auto supplemental_package = make_package(
            make_asset_map("10000000-0000-0000-0000-000000000208",
                           {
                               make_asset_map_asset("30000000-0000-0000-0000-000000000208", "CPL/supp.xml"),
                               make_asset_map_asset("32000000-0000-0000-0000-000000000208", "MXF/supp_picture.mxf"),
                           }),
            make_pkl("20000000-0000-0000-0000-000000000208",
                     {
                         make_pkl_asset("30000000-0000-0000-0000-000000000208",
                                        AssetType::composition_playlist,
                                        "CPL/supp.xml"),
                         make_pkl_asset("32000000-0000-0000-0000-000000000208",
                                        AssetType::track_file,
                                        "MXF/supp_picture.mxf"),
                     }),
            {
                make_cpl("30000000-0000-0000-0000-000000000208",
                         "Feature_Supp_Not_Allowed",
                         {
                             Reel{
                                 .reel_id = base_reel_id,
                                 .picture = make_reference("32000000-0000-0000-0000-000000000208", TrackType::picture),
                             },
                         }),
            });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {std::move(supplemental_package)},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "not-allowed",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000208",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::sound},
                    },
                },
        });

        expect(!result.ok(), "Disallowed override must fail");
        expect_single_diagnostic(result,
                                 "supplemental.override_not_allowed",
                                 "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "duplicate-override",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000299",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {TrackType::picture, TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Duplicate allowed_overrides must fail");
        expect_single_diagnostic(result,
                                 "supplemental.duplicate_allowed_override",
                                 "/SupplementalMergePolicyList/Policy[1]/AllowedOverrides[2]");
    }

    {
        const auto base_graph =
            resolve_base_graph(base_composition_id,
                               {
                                   make_base_package(base_composition_id, base_reel_id, base_picture_id, base_sound_id),
                               });

        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = base_graph,
            .packages = {},
            .policies =
                {
                    MergePolicy{
                        .policy_id = "invalid-override",
                        .base_composition_id = base_composition_id,
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000299",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {static_cast<TrackType>(99)},
                    },
                },
        });

        expect(!result.ok(), "Invalid allowed_overrides value must fail");
        expect_single_diagnostic(result,
                                 "supplemental.invalid_allowed_override",
                                 "/SupplementalMergePolicyList/Policy[1]/AllowedOverrides[1]");
    }

    return 0;
}
