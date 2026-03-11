#include "asset_map.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using dcplayer::dcp::assetmap::Diagnostic;
using dcplayer::dcp::assetmap::DiagnosticSeverity;

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
    expect(diagnostics.size() == 1U, "Expected exactly one diagnostic");
    expect(diagnostics[0].code == code, "Unexpected diagnostic code");
    expect(diagnostics[0].severity == severity, "Unexpected diagnostic severity");
    expect(diagnostics[0].path == path, "Unexpected diagnostic path");
}

}  // namespace

int main() {
    using dcplayer::dcp::assetmap::DiagnosticSeverity;
    using dcplayer::dcp::assetmap::SchemaFlavor;
    using dcplayer::dcp::assetmap::ValidationStatus;

    const std::filesystem::path fixture_root{ASSETMAP_FIXTURE_DIR};

    const auto valid_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "valid" / "interop_assetmap.xml"));
    expect(valid_result.ok(), "Valid AssetMap fixture should parse");
    expect(valid_result.validation_status == ValidationStatus::ok, "Expected clean validation status");
    expect(valid_result.diagnostics.empty(), "Expected no diagnostics");
    expect(valid_result.asset_map.has_value(), "Expected normalized AssetMap");
    expect(valid_result.asset_map->asset_map_id == "11111111-1111-1111-1111-111111111111", "Unexpected asset_map_id");
    expect(valid_result.asset_map->schema_flavor == SchemaFlavor::interop, "Unexpected schema flavor");
    expect(valid_result.asset_map->assets.size() == 3U, "Unexpected asset count");
    expect(valid_result.asset_map->assets[0].packing_list_hint, "Expected packing_list_hint");
    expect(valid_result.asset_map->assets[0].is_text_xml, "Expected XML hint for PKL");
    expect(valid_result.asset_map->assets[2].chunk_list[0].path == "MXF/reel_01_picture.mxf", "Unexpected normalized path");

    const auto warning_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "valid" / "unsupported_namespace.xml"));
    expect(warning_result.ok(), "Unknown namespace should stay parseable");
    expect(warning_result.validation_status == ValidationStatus::warning, "Expected warning status");
    expect_single_diagnostic(warning_result.diagnostics,
                             "assetmap.unsupported_namespace",
                             DiagnosticSeverity::warning,
                             "/AssetMap");

    const auto duplicate_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "invalid" / "duplicate_asset_id.xml"));
    expect(!duplicate_result.ok(), "Duplicate asset_id must fail validation");
    expect(duplicate_result.validation_status == ValidationStatus::error, "Expected error status");
    expect_single_diagnostic(duplicate_result.diagnostics,
                             "assetmap.duplicate_asset_id",
                             DiagnosticSeverity::error,
                             "/AssetMap/AssetList/Asset[2]/Id");

    const auto invalid_path_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "invalid" / "invalid_relative_path.xml"));
    expect(!invalid_path_result.ok(), "Invalid relative path must fail validation");
    expect_single_diagnostic(invalid_path_result.diagnostics,
                             "assetmap.invalid_relative_path",
                             DiagnosticSeverity::error,
                             "/AssetMap/AssetList/Asset[1]/ChunkList/Chunk[1]/Path");

    const auto decimal_entity_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "valid" / "decimal_entity_path.xml"));
    expect(decimal_entity_result.ok(), "Decimal numeric entity path should parse");
    expect(decimal_entity_result.asset_map.has_value(), "Expected normalized AssetMap for decimal entity path");
    expect(decimal_entity_result.asset_map->assets[0].chunk_list[0].path == "CPL/main_cpl.xml",
           "Decimal entity path must be decoded before normalization");

    const auto hex_entity_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "valid" / "hex_entity_path.xml"));
    expect(hex_entity_result.ok(), "Hex numeric entity path should parse");
    expect(hex_entity_result.asset_map.has_value(), "Expected normalized AssetMap for hex entity path");
    expect(hex_entity_result.asset_map->assets[0].chunk_list[0].path == "CPL/main_cpl.xml",
           "Hex entity path must be decoded before normalization");

    const auto traversal_decimal_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "invalid" / "traversal_decimal_entity.xml"));
    expect(!traversal_decimal_result.ok(), "Decimal numeric traversal path must fail validation");
    expect_single_diagnostic(traversal_decimal_result.diagnostics,
                             "assetmap.invalid_relative_path",
                             DiagnosticSeverity::error,
                             "/AssetMap/AssetList/Asset[1]/ChunkList/Chunk[1]/Path");

    const auto traversal_hex_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "invalid" / "traversal_hex_entity.xml"));
    expect(!traversal_hex_result.ok(), "Hex numeric traversal path must fail validation");
    expect_single_diagnostic(traversal_hex_result.diagnostics,
                             "assetmap.invalid_relative_path",
                             DiagnosticSeverity::error,
                             "/AssetMap/AssetList/Asset[1]/ChunkList/Chunk[1]/Path");

    const auto malformed_entity_result =
        dcplayer::dcp::assetmap::parse(read_fixture(fixture_root / "invalid" / "malformed_numeric_entity.xml"));
    expect(!malformed_entity_result.ok(), "Malformed numeric entity must fail parsing");
    expect_single_diagnostic(malformed_entity_result.diagnostics,
                             "assetmap.xml_malformed",
                             DiagnosticSeverity::error,
                             "/AssetMap/AssetList/Asset/ChunkList/Chunk/Path");

    const auto bom_result =
        dcplayer::dcp::assetmap::parse(std::string{"\xEF\xBB\xBF"} + read_fixture(fixture_root / "valid" / "interop_assetmap.xml"));
    expect(bom_result.ok(), "UTF-8 BOM-prefixed AssetMap must parse");
    expect(bom_result.asset_map.has_value(), "Expected normalized AssetMap for BOM fixture");

    return 0;
}
