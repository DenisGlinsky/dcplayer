#include "composition_playlist.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using dcplayer::dcp::cpl::Diagnostic;
using dcplayer::dcp::cpl::DiagnosticSeverity;

std::string read_fixture(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open fixture: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expect_single_diagnostic(const std::vector<Diagnostic>& diagnostics,
                              const std::string& code,
                              DiagnosticSeverity severity,
                              const std::string& path) {
    expect(diagnostics.size() == 1U, "Expected exactly one CPL diagnostic");
    expect(diagnostics[0].code == code, "Unexpected diagnostic code");
    expect(diagnostics[0].severity == severity, "Unexpected diagnostic severity");
    expect(diagnostics[0].path == path, "Unexpected diagnostic path");
}

}  // namespace

int main() {
    using dcplayer::dcp::cpl::DiagnosticSeverity;
    using dcplayer::dcp::cpl::SchemaFlavor;
    using dcplayer::dcp::cpl::TrackType;
    using dcplayer::dcp::cpl::ValidationStatus;

    const std::filesystem::path fixture_root{CPL_FIXTURE_DIR};

    const auto valid_result = dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "valid" / "interop_cpl.xml"));
    expect(valid_result.ok(), "Valid CPL fixture should parse");
    expect(valid_result.validation_status == ValidationStatus::ok, "Expected clean validation status");
    expect(valid_result.diagnostics.empty(), "Expected no diagnostics");
    expect(valid_result.composition_playlist.has_value(), "Expected normalized CPL");
    expect(valid_result.composition_playlist->composition_id == "77777777-7777-7777-7777-777777777777",
           "Unexpected composition_id");
    expect(valid_result.composition_playlist->schema_flavor == SchemaFlavor::interop, "Unexpected schema flavor");
    expect(valid_result.composition_playlist->content_title_text == "Feature_FTR-1_S_EN-XX_OV",
           "Unexpected content title");
    expect(valid_result.composition_playlist->edit_rate.numerator == 24U, "Unexpected composition edit rate numerator");
    expect(valid_result.composition_playlist->edit_rate.denominator == 1U,
           "Unexpected composition edit rate denominator");
    expect(valid_result.composition_playlist->reels.size() == 2U, "Unexpected reel count");
    expect(valid_result.composition_playlist->reels[0].picture.has_value(), "Expected picture reference");
    expect(valid_result.composition_playlist->reels[0].picture->track_type == TrackType::picture,
           "Unexpected picture track type");
    expect(valid_result.composition_playlist->reels[0].picture->asset_id == "33333333-3333-3333-3333-333333333333",
           "Unexpected picture asset_id");
    expect(valid_result.composition_playlist->reels[0].sound.has_value(), "Expected sound reference");
    expect(valid_result.composition_playlist->reels[0].subtitle.has_value(), "Expected subtitle reference");
    expect(valid_result.composition_playlist->reels[1].sound == std::nullopt, "Second reel should not contain sound");

    const auto warning_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "valid" / "unsupported_namespace.xml"));
    expect(warning_result.ok(), "Unknown CPL namespace should stay parseable");
    expect(warning_result.validation_status == ValidationStatus::warning, "Expected warning status");
    expect_single_diagnostic(warning_result.diagnostics,
                             "cpl.unsupported_namespace",
                             DiagnosticSeverity::warning,
                             "/CompositionPlaylist");

    const auto missing_reel_list_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "missing_reel_list.xml"));
    expect(!missing_reel_list_result.ok(), "Missing ReelList must fail validation");
    expect_single_diagnostic(missing_reel_list_result.diagnostics,
                             "cpl.missing_required_field",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ReelList");

    const auto invalid_edit_rate_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "invalid_edit_rate.xml"));
    expect(!invalid_edit_rate_result.ok(), "Invalid composition EditRate must fail validation");
    expect_single_diagnostic(invalid_edit_rate_result.diagnostics,
                             "cpl.invalid_edit_rate",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/EditRate");

    const auto duplicate_reel_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "duplicate_reel_id.xml"));
    expect(!duplicate_reel_result.ok(), "Duplicate reel_id must fail validation");
    expect_single_diagnostic(duplicate_reel_result.diagnostics,
                             "cpl.duplicate_reel_id",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ReelList/Reel[2]/Id");

    const auto invalid_asset_uuid_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "invalid_asset_uuid.xml"));
    expect(!invalid_asset_uuid_result.ok(), "Invalid asset UUID must fail validation");
    expect_single_diagnostic(invalid_asset_uuid_result.diagnostics,
                             "cpl.invalid_uuid",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture/Id");

    const auto duplicate_track_type_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "duplicate_track_type.xml"));
    expect(!duplicate_track_type_result.ok(), "Duplicate track type must fail validation");
    expect_single_diagnostic(duplicate_track_type_result.diagnostics,
                             "cpl.duplicate_track_type",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainPicture[2]");

    const auto duplicate_asset_reference_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "duplicate_asset_reference.xml"));
    expect(!duplicate_asset_reference_result.ok(), "Duplicate asset reference must fail validation");
    expect_single_diagnostic(duplicate_asset_reference_result.diagnostics,
                             "cpl.duplicate_asset_reference",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ReelList/Reel[1]/AssetList/MainSound/Id");

    const auto reel_without_tracks_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "reel_without_tracks.xml"));
    expect(!reel_without_tracks_result.ok(), "Reel without supported tracks must fail validation");
    expect_single_diagnostic(reel_without_tracks_result.diagnostics,
                             "cpl.reel_missing_track",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ReelList/Reel[1]/AssetList");

    const auto malformed_entity_result =
        dcplayer::dcp::cpl::parse(read_fixture(fixture_root / "invalid" / "malformed_numeric_entity.xml"));
    expect(!malformed_entity_result.ok(), "Malformed numeric entity must fail parsing");
    expect_single_diagnostic(malformed_entity_result.diagnostics,
                             "cpl.xml_malformed",
                             DiagnosticSeverity::error,
                             "/CompositionPlaylist/ContentTitleText");

    const auto bom_result =
        dcplayer::dcp::cpl::parse(std::string{"\xEF\xBB\xBF"} + read_fixture(fixture_root / "valid" / "interop_cpl.xml"));
    expect(bom_result.ok(), "UTF-8 BOM-prefixed CPL must parse");
    expect(bom_result.composition_playlist.has_value(), "Expected normalized CPL for BOM fixture");

    return 0;
}
