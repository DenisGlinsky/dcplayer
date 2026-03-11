#include "internal/xml_dom.hpp"

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace dcplayer::dcp::xml {
namespace {

constexpr std::string_view kWhitespace = " \t\r\n";

[[nodiscard]] std::string trim_copy(std::string_view input) {
    const auto begin = input.find_first_not_of(kWhitespace);
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = input.find_last_not_of(kWhitespace);
    return std::string{input.substr(begin, end - begin + 1U)};
}

[[nodiscard]] std::string local_name_for(std::string_view raw_name) {
    const auto separator = raw_name.find(':');
    if (separator == std::string_view::npos) {
        return std::string{raw_name};
    }
    return std::string{raw_name.substr(separator + 1U)};
}

class Parser {
  public:
    explicit Parser(std::string_view input) : input_(input) {}

    [[nodiscard]] ParseResult parse() {
        if (starts_with("\xEF\xBB\xBF")) {
            pos_ += 3U;
        }
        skip_misc();
        if (pos_ >= input_.size()) {
            return {.error = ParseError{"/", "empty XML document"}};
        }

        auto root = parse_element();
        if (!root.has_value()) {
            return {.error = ParseError{error_path_, error_message_}};
        }

        skip_misc();
        if (pos_ != input_.size()) {
            return {.error = ParseError{current_path(), "unexpected trailing content"}};
        }

        return {.document = Document{std::move(*root)}};
    }

  private:
    struct PathGuard {
        explicit PathGuard(std::vector<std::string>& stack) : stack_(stack) {}
        ~PathGuard() {
            stack_.pop_back();
        }

        std::vector<std::string>& stack_;
    };

    [[nodiscard]] std::optional<char32_t> parse_numeric_entity(std::string_view body, std::string path) {
        if (body.empty()) {
            set_error("empty numeric XML entity", std::move(path));
            return std::nullopt;
        }

        std::string_view digits = body;
        int base = 10;
        if (body.front() == 'x' || body.front() == 'X') {
            digits.remove_prefix(1U);
            base = 16;
        }
        if (digits.empty()) {
            set_error("empty numeric XML entity", std::move(path));
            return std::nullopt;
        }

        std::uint32_t code_point = 0U;
        for (const char current : digits) {
            std::uint32_t digit = 0U;
            if (base == 10) {
                if (std::isdigit(static_cast<unsigned char>(current)) == 0) {
                    set_error("invalid decimal numeric XML entity", std::move(path));
                    return std::nullopt;
                }
                digit = static_cast<std::uint32_t>(current - '0');
            } else {
                if (current >= '0' && current <= '9') {
                    digit = static_cast<std::uint32_t>(current - '0');
                } else {
                    const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(current)));
                    if (lower < 'a' || lower > 'f') {
                        set_error("invalid hexadecimal numeric XML entity", std::move(path));
                        return std::nullopt;
                    }
                    digit = static_cast<std::uint32_t>(10 + lower - 'a');
                }
            }

            constexpr std::uint32_t kMaxCodePoint = 0x10FFFFU;
            if (code_point > (kMaxCodePoint - digit) / static_cast<std::uint32_t>(base)) {
                set_error("numeric XML entity overflow", std::move(path));
                return std::nullopt;
            }
            code_point = code_point * static_cast<std::uint32_t>(base) + digit;
        }

        const bool invalid_control = code_point < 0x20U && code_point != 0x09U && code_point != 0x0AU && code_point != 0x0DU;
        const bool surrogate = code_point >= 0xD800U && code_point <= 0xDFFFU;
        if (code_point == 0U || code_point > 0x10FFFFU || surrogate || invalid_control) {
            set_error("invalid Unicode code point in XML entity", std::move(path));
            return std::nullopt;
        }
        return static_cast<char32_t>(code_point);
    }

    [[nodiscard]] std::string encode_utf8(char32_t code_point) {
        std::string output;
        if (code_point <= 0x7FU) {
            output.push_back(static_cast<char>(code_point));
            return output;
        }
        if (code_point <= 0x7FFU) {
            output.push_back(static_cast<char>(0xC0U | ((code_point >> 6U) & 0x1FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
            return output;
        }
        if (code_point <= 0xFFFFU) {
            output.push_back(static_cast<char>(0xE0U | ((code_point >> 12U) & 0x0FU)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
            return output;
        }

        output.push_back(static_cast<char>(0xF0U | ((code_point >> 18U) & 0x07U)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        return output;
    }

    [[nodiscard]] std::optional<std::string> decode_entities(std::string_view input, std::string path) {
        std::string output;
        output.reserve(input.size());

        for (std::size_t index = 0U; index < input.size(); ++index) {
            const char current = input[index];
            if (current != '&') {
                output.push_back(current);
                continue;
            }

            const auto semicolon = input.find(';', index);
            if (semicolon == std::string_view::npos) {
                set_error("unterminated XML entity", std::move(path));
                return std::nullopt;
            }

            const auto entity = input.substr(index, semicolon - index + 1U);
            if (entity == "&lt;") {
                output.push_back('<');
            } else if (entity == "&gt;") {
                output.push_back('>');
            } else if (entity == "&amp;") {
                output.push_back('&');
            } else if (entity == "&quot;") {
                output.push_back('"');
            } else if (entity == "&apos;") {
                output.push_back('\'');
            } else if (entity.size() >= 4U && entity[1] == '#') {
                const auto code_point = parse_numeric_entity(entity.substr(2U, entity.size() - 3U), path);
                if (!code_point.has_value()) {
                    return std::nullopt;
                }
                output += encode_utf8(*code_point);
            } else {
                set_error("unsupported XML entity", std::move(path));
                return std::nullopt;
            }

            index = semicolon;
        }

        return output;
    }

    [[nodiscard]] std::string current_path() const {
        if (path_stack_.empty()) {
            return "/";
        }

        std::string path;
        for (const auto& component : path_stack_) {
            path += "/";
            path += component;
        }
        return path;
    }

    [[nodiscard]] std::optional<Node> parse_element() {
        if (!consume('<')) {
            set_error("expected '<' to start element", current_path());
            return std::nullopt;
        }

        if (match("!--")) {
            set_error("comment is not a valid root element", current_path());
            return std::nullopt;
        }
        if (match("?")) {
            set_error("processing instruction is not a valid root element", current_path());
            return std::nullopt;
        }

        const std::string raw_name = parse_name();
        if (raw_name.empty()) {
            set_error("expected element name", current_path());
            return std::nullopt;
        }

        Node node;
        node.raw_name = raw_name;
        node.local_name = local_name_for(raw_name);
        path_stack_.push_back(node.local_name);
        PathGuard path_guard(path_stack_);

        skip_whitespace();
        while (pos_ < input_.size() && input_[pos_] != '>' && !starts_with("/>")) {
            const std::string attribute_name = parse_name();
            if (attribute_name.empty()) {
                set_error("expected attribute name", current_path());
                return std::nullopt;
            }

            skip_whitespace();
            if (!consume('=')) {
                set_error("expected '=' after attribute name", current_path() + "/@" + attribute_name);
                return std::nullopt;
            }
            skip_whitespace();

            const auto attribute_value = parse_attribute_value(current_path() + "/@" + attribute_name);
            if (!attribute_value.has_value()) {
                return std::nullopt;
            }

            node.attributes.push_back(Attribute{attribute_name, *attribute_value});
            skip_whitespace();
        }

        if (consume('/') && consume('>')) {
            return node;
        }

        if (!consume('>')) {
            set_error("expected '>' to close start tag", current_path());
            return std::nullopt;
        }

        while (pos_ < input_.size()) {
            if (starts_with("</")) {
                pos_ += 2U;
                const std::string closing_name = parse_name();
                if (closing_name != raw_name) {
                    set_error("mismatched closing tag", current_path());
                    return std::nullopt;
                }
                skip_whitespace();
                if (!consume('>')) {
                    set_error("expected '>' to close end tag", current_path());
                    return std::nullopt;
                }
                return node;
            }

            if (starts_with("<!--")) {
                if (!skip_comment()) {
                    return std::nullopt;
                }
                continue;
            }

            if (starts_with("<?")) {
                if (!skip_processing_instruction()) {
                    return std::nullopt;
                }
                continue;
            }

            if (peek() == '<') {
                auto child = parse_element();
                if (!child.has_value()) {
                    return std::nullopt;
                }
                node.children.push_back(std::move(*child));
                continue;
            }

            const auto text = parse_text(current_path());
            if (!text.has_value()) {
                return std::nullopt;
            }
            node.text.append(*text);
        }

        set_error("unexpected end of input", current_path());
        return std::nullopt;
    }

    [[nodiscard]] std::string parse_name() {
        const auto start = pos_;
        while (pos_ < input_.size()) {
            const char current = input_[pos_];
            const bool valid = std::isalnum(static_cast<unsigned char>(current)) != 0 || current == '_' || current == '-' ||
                               current == ':' || current == '.';
            if (!valid) {
                break;
            }
            ++pos_;
        }

        return std::string{input_.substr(start, pos_ - start)};
    }

    [[nodiscard]] std::optional<std::string> parse_attribute_value(std::string path) {
        if (pos_ >= input_.size()) {
            set_error("expected quoted attribute value", std::move(path));
            return std::nullopt;
        }

        const char quote = input_[pos_];
        if (quote != '"' && quote != '\'') {
            set_error("expected quoted attribute value", std::move(path));
            return std::nullopt;
        }
        ++pos_;

        const auto start = pos_;
        while (pos_ < input_.size() && input_[pos_] != quote) {
            ++pos_;
        }
        if (pos_ >= input_.size()) {
            set_error("unterminated attribute value", std::move(path));
            return std::nullopt;
        }

        auto value = decode_entities(input_.substr(start, pos_ - start), std::move(path));
        if (!value.has_value()) {
            return std::nullopt;
        }
        ++pos_;
        return *value;
    }

    [[nodiscard]] std::optional<std::string> parse_text(std::string path) {
        const auto start = pos_;
        while (pos_ < input_.size() && input_[pos_] != '<') {
            ++pos_;
        }
        return decode_entities(input_.substr(start, pos_ - start), std::move(path));
    }

    void skip_whitespace() {
        while (pos_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[pos_])) != 0) {
            ++pos_;
        }
    }

    void skip_misc() {
        while (true) {
            skip_whitespace();
            if (starts_with("<?")) {
                if (!skip_processing_instruction()) {
                    return;
                }
                continue;
            }
            if (starts_with("<!--")) {
                if (!skip_comment()) {
                    return;
                }
                continue;
            }
            break;
        }
    }

    [[nodiscard]] bool skip_comment() {
        if (!starts_with("<!--")) {
            return true;
        }
        const auto end = input_.find("-->", pos_);
        if (end == std::string_view::npos) {
            set_error("unterminated comment", current_path());
            return false;
        }
        pos_ = end + 3U;
        return true;
    }

    [[nodiscard]] bool skip_processing_instruction() {
        if (!starts_with("<?")) {
            return true;
        }
        const auto end = input_.find("?>", pos_);
        if (end == std::string_view::npos) {
            set_error("unterminated processing instruction", current_path());
            return false;
        }
        pos_ = end + 2U;
        return true;
    }

    [[nodiscard]] char peek() const noexcept {
        if (pos_ >= input_.size()) {
            return '\0';
        }
        return input_[pos_];
    }

    [[nodiscard]] bool consume(char expected) {
        if (pos_ < input_.size() && input_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool starts_with(std::string_view prefix) const noexcept {
        return input_.substr(pos_, prefix.size()) == prefix;
    }

    [[nodiscard]] bool match(std::string_view text) {
        if (!starts_with(text)) {
            return false;
        }
        pos_ += text.size();
        return true;
    }

    void set_error(std::string message, std::string path) {
        if (error_message_.empty()) {
            error_message_ = std::move(message);
            error_path_ = std::move(path);
        }
    }

    std::string_view input_;
    std::size_t pos_{0U};
    std::string error_message_;
    std::string error_path_;
    std::vector<std::string> path_stack_;
};

}  // namespace

const Attribute* Node::find_attribute(std::string_view name) const noexcept {
    for (const auto& attribute : attributes) {
        if (attribute.name == name) {
            return &attribute;
        }
    }
    return nullptr;
}

std::optional<std::string> Node::namespace_uri() const {
    const auto separator = raw_name.find(':');
    if (separator == std::string::npos) {
        if (const auto* attribute = find_attribute("xmlns"); attribute != nullptr) {
            return attribute->value;
        }
        return std::nullopt;
    }

    const std::string prefix = raw_name.substr(0U, separator);
    const std::string attribute_name = "xmlns:" + prefix;
    if (const auto* attribute = find_attribute(attribute_name); attribute != nullptr) {
        return attribute->value;
    }
    if (const auto* attribute = find_attribute("xmlns"); attribute != nullptr) {
        return attribute->value;
    }
    return std::nullopt;
}

ParseResult parse_document(std::string_view xml_text) {
    Parser parser(xml_text);
    return parser.parse();
}

std::string trimmed_text(const Node& node) {
    return trim_copy(node.text);
}

const Node* find_child(const Node& node, std::string_view local_name) noexcept {
    for (const auto& child : node.children) {
        if (child.local_name == local_name) {
            return &child;
        }
    }
    return nullptr;
}

std::vector<const Node*> find_children(const Node& node, std::string_view local_name) {
    std::vector<const Node*> matches;
    for (const auto& child : node.children) {
        if (child.local_name == local_name) {
            matches.push_back(&child);
        }
    }
    return matches;
}

}  // namespace dcplayer::dcp::xml
