#include "asset_map.hpp"
#include "packing_list.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

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

void expect_single_parse_diagnostic(const std::vector<dcplayer::dcp::pkl::Diagnostic>& diagnostics,
                                    const std::string& code,
                                    dcplayer::dcp::pkl::DiagnosticSeverity severity,
                                    const std::string& path) {
    expect(diagnostics.size() == 1U, "Expected exactly one PKL diagnostic");
    expect(diagnostics[0].code == code, "Unexpected diagnostic code");
    expect(diagnostics[0].severity == severity, "Unexpected diagnostic severity");
    expect(diagnostics[0].path == path, "Unexpected diagnostic path");
}

}  // namespace

int main() {
    using dcplayer::dcp::pkl::AssetType;
    using dcplayer::dcp::pkl::DiagnosticSeverity;
    using dcplayer::dcp::pkl::DigestAlgorithm;
    using dcplayer::dcp::pkl::SchemaFlavor;
    using dcplayer::dcp::pkl::ValidationStatus;

    const std::filesystem::path assetmap_root{ASSETMAP_FIXTURE_DIR};
    const std::filesystem::path pkl_root{PKL_FIXTURE_DIR};

    const auto valid_result = dcplayer::dcp::pkl::parse(read_fixture(pkl_root / "valid" / "interop_pkl.xml"));
    expect(valid_result.ok(), "Valid PKL fixture should parse");
    expect(valid_result.validation_status == ValidationStatus::ok, "Expected clean validation status");
    expect(valid_result.diagnostics.empty(), "Expected no diagnostics");
    expect(valid_result.packing_list.has_value(), "Expected normalized PKL");
    expect(valid_result.packing_list->schema_flavor == SchemaFlavor::interop, "Unexpected schema flavor");
    expect(valid_result.packing_list->assets.size() == 2U, "Unexpected PKL asset count");
    expect(valid_result.packing_list->assets[0].asset_type == AssetType::composition_playlist, "Unexpected CPL asset type");
    expect(valid_result.packing_list->assets[0].hash.algorithm == DigestAlgorithm::sha1, "Unexpected digest algorithm");
    expect(valid_result.packing_list->assets[0].hash.normalized_hex ==
               std::optional<std::string>{"00112233445566778899aabbccddeeff00112233"},
           "Unexpected normalized SHA-1 digest");
    expect(valid_result.packing_list->assets[1].hash.normalized_hex ==
               std::optional<std::string>{"00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"},
           "Unexpected normalized SHA-256 digest");

    const auto warning_result = dcplayer::dcp::pkl::parse(read_fixture(pkl_root / "valid" / "unsupported_namespace.xml"));
    expect(warning_result.ok(), "Unknown PKL namespace should stay parseable");
    expect(warning_result.validation_status == ValidationStatus::warning, "Expected warning status");
    expect_single_parse_diagnostic(warning_result.diagnostics,
                                   "pkl.unsupported_namespace",
                                   DiagnosticSeverity::warning,
                                   "/PackingList");

    const auto duplicate_result =
        dcplayer::dcp::pkl::parse(read_fixture(pkl_root / "invalid" / "duplicate_asset_id.xml"));
    expect(!duplicate_result.ok(), "Duplicate PKL asset_id must fail validation");
    expect_single_parse_diagnostic(duplicate_result.diagnostics,
                                   "pkl.duplicate_asset_id",
                                   DiagnosticSeverity::error,
                                   "/PackingList/AssetList/Asset[2]/Id");

    const auto invalid_algorithm_result =
        dcplayer::dcp::pkl::parse(read_fixture(pkl_root / "invalid" / "invalid_digest_algorithm.xml"));
    expect(!invalid_algorithm_result.ok(), "Unknown digest algorithm must fail validation");
    expect(invalid_algorithm_result.diagnostics.size() == 2U, "Expected algorithm and value diagnostics");
    expect(invalid_algorithm_result.diagnostics[0].code == "pkl.invalid_digest_algorithm", "Expected algorithm error first");
    expect(invalid_algorithm_result.diagnostics[0].path == "/PackingList/AssetList/Asset[1]/Hash/@Algorithm",
           "Unexpected algorithm diagnostic path");
    expect(invalid_algorithm_result.diagnostics[1].code == "pkl.invalid_digest_value", "Expected digest value error second");

    const auto invalid_value_result =
        dcplayer::dcp::pkl::parse(read_fixture(pkl_root / "invalid" / "invalid_digest_value.xml"));
    expect(!invalid_value_result.ok(), "Invalid digest value must fail validation");
    expect_single_parse_diagnostic(invalid_value_result.diagnostics,
                                   "pkl.invalid_digest_value",
                                   DiagnosticSeverity::error,
                                   "/PackingList/AssetList/Asset[1]/Hash");

    const auto asset_map_result =
        dcplayer::dcp::assetmap::parse(read_fixture(assetmap_root / "valid" / "interop_assetmap.xml"));
    expect(asset_map_result.ok() && asset_map_result.asset_map.has_value(), "Expected valid AssetMap for cross-check");

    const auto membership_result =
        dcplayer::dcp::pkl::parse(read_fixture(pkl_root / "valid" / "missing_in_assetmap.xml"));
    expect(membership_result.ok() && membership_result.packing_list.has_value(), "Expected PKL parse before cross-validation");

    const auto membership_diagnostics =
        dcplayer::dcp::pkl::validate_against_asset_map(*membership_result.packing_list, *asset_map_result.asset_map);
    expect(membership_diagnostics.size() == 1U, "Expected one cross-validation diagnostic");
    expect(membership_diagnostics[0].code == "pkl.asset_missing_in_assetmap", "Unexpected cross-validation code");
    expect(membership_diagnostics[0].path == "/PackingList/AssetList/Asset[1]/Id", "Unexpected cross-validation path");

    const auto bom_result =
        dcplayer::dcp::pkl::parse(std::string{"\xEF\xBB\xBF"} + read_fixture(pkl_root / "valid" / "interop_pkl.xml"));
    expect(bom_result.ok(), "UTF-8 BOM-prefixed PKL must parse");
    expect(bom_result.packing_list.has_value(), "Expected normalized PKL for BOM fixture");

    return 0;
}
