#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::dcp::xml {

struct Attribute {
    std::string name;
    std::string value;
};

struct Node {
    std::string raw_name;
    std::string local_name;
    std::vector<Attribute> attributes;
    std::vector<Node> children;
    std::string text;

    [[nodiscard]] const Attribute* find_attribute(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<std::string> namespace_uri() const;
};

struct ParseError {
    std::string path;
    std::string message;
};

struct Document {
    Node root;
};

struct ParseResult {
    std::optional<Document> document;
    std::optional<ParseError> error;

    [[nodiscard]] bool ok() const noexcept {
        return document.has_value() && !error.has_value();
    }
};

[[nodiscard]] ParseResult parse_document(std::string_view xml_text);
[[nodiscard]] std::string trimmed_text(const Node& node);
[[nodiscard]] const Node* find_child(const Node& node, std::string_view local_name) noexcept;
[[nodiscard]] std::vector<const Node*> find_children(const Node& node, std::string_view local_name);

}  // namespace dcplayer::dcp::xml
