#include <optiweave/runtime/json_parser.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace optiweave {
namespace json {

namespace {

class JsonParser {
public:
    explicit JsonParser(const std::string& input) : input_(input), pos_(0) {}

    ParsedDashboard parse() {
        ParsedDashboard dashboard;
        skip_whitespace();
        expect('{');

        while (peek() != '}') {
            std::string key = parse_string();
            skip_whitespace();
            expect(':');

            if (key == "summary") {
                dashboard.summary = parse_summary();
            } else if (key == "hotspots") {
                dashboard.hotspots = parse_hotspots();
            } else if (key == "operations") {
                dashboard.operations = parse_operations();
            } else if (key == "suggestions") {
                dashboard.suggestions = parse_suggestions();
            } else {
                skip_value();
            }

            skip_whitespace();
            if (peek() == ',') advance();
        }

        expect('}');
        return dashboard;
    }

private:
    const std::string& input_;
    size_t pos_;

    char peek() const {
        if (pos_ >= input_.size()) throw std::runtime_error("Unexpected end of JSON");
        return input_[pos_];
    }

    char advance() {
        if (pos_ >= input_.size()) throw std::runtime_error("Unexpected end of JSON");
        return input_[pos_++];
    }

    void expect(char c) {
        skip_whitespace();
        char got = advance();
        if (got != c) {
            throw std::runtime_error(std::string("Expected '") + c + "' but got '" + got +
                                     "' at position " + std::to_string(pos_ - 1));
        }
    }

    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            pos_++;
        }
    }

    std::string parse_string() {
        skip_whitespace();
        expect('"');
        std::string result;
        while (pos_ < input_.size() && input_[pos_] != '"') {
            if (input_[pos_] == '\\') {
                pos_++;
                if (pos_ >= input_.size()) throw std::runtime_error("Unexpected end of string escape");
                switch (input_[pos_]) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u': {
                        // Skip unicode escapes — just consume 4 hex digits
                        pos_++;
                        for (int i = 0; i < 4 && pos_ < input_.size(); i++) pos_++;
                        pos_--; // will be incremented below
                        result += '?';
                        break;
                    }
                    default: result += input_[pos_]; break;
                }
            } else {
                result += input_[pos_];
            }
            pos_++;
        }
        if (pos_ >= input_.size()) throw std::runtime_error("Unterminated string");
        pos_++; // skip closing quote
        return result;
    }

    // Parse a raw number token as string (integer or floating point)
    std::string parse_number_token() {
        skip_whitespace();
        size_t start = pos_;
        if (pos_ < input_.size() && input_[pos_] == '-') pos_++;
        while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) pos_++;
        if (pos_ < input_.size() && input_[pos_] == '.') {
            pos_++;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) pos_++;
        }
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            pos_++;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) pos_++;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) pos_++;
        }
        return input_.substr(start, pos_ - start);
    }

    int64_t parse_int() {
        std::string token = parse_number_token();
        return std::stoll(token);
    }

    uint64_t parse_uint() {
        std::string token = parse_number_token();
        return std::stoull(token);
    }

    double parse_double() {
        std::string token = parse_number_token();
        return std::stod(token);
    }

    // Parse a number that could be int or float, return as double
    double parse_number_as_double() {
        std::string token = parse_number_token();
        return std::stod(token);
    }

    void skip_value() {
        skip_whitespace();
        char c = peek();
        if (c == '"') {
            parse_string();
        } else if (c == '{') {
            advance();
            while (peek() != '}') {
                parse_string(); // key
                skip_whitespace();
                expect(':');
                skip_value();
                skip_whitespace();
                if (peek() == ',') advance();
            }
            advance(); // }
        } else if (c == '[') {
            advance();
            while (peek() != ']') {
                skip_value();
                skip_whitespace();
                if (peek() == ',') advance();
            }
            advance(); // ]
        } else if (c == 't') {
            // true
            pos_ += 4;
        } else if (c == 'f') {
            // false
            pos_ += 5;
        } else if (c == 'n') {
            // null
            pos_ += 4;
        } else {
            // number
            parse_number_token();
        }
    }

    std::vector<std::string> parse_string_array() {
        std::vector<std::string> result;
        skip_whitespace();
        expect('[');
        skip_whitespace();
        if (peek() != ']') {
            while (true) {
                result.push_back(parse_string());
                skip_whitespace();
                if (peek() == ',') {
                    advance();
                } else {
                    break;
                }
            }
        }
        expect(']');
        return result;
    }

    ParsedSummary parse_summary() {
        ParsedSummary summary;
        skip_whitespace();
        expect('{');
        while (peek() != '}') {
            std::string key = parse_string();
            skip_whitespace();
            expect(':');

            if (key == "total_operations") {
                summary.total_operations = parse_uint();
            } else if (key == "total_runtime_ns") {
                summary.total_runtime_ns = parse_uint();
            } else if (key == "total_runtime_ms") {
                summary.total_runtime_ms = parse_double();
            } else if (key == "issues_found") {
                summary.issues_found = static_cast<size_t>(parse_uint());
            } else if (key == "potential_speedup") {
                summary.potential_speedup = parse_string();
            } else {
                skip_value();
            }

            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect('}');
        return summary;
    }

    std::vector<ParsedHotspot> parse_hotspots() {
        std::vector<ParsedHotspot> hotspots;
        skip_whitespace();
        expect('[');
        skip_whitespace();
        while (peek() != ']') {
            hotspots.push_back(parse_single_hotspot());
            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect(']');
        return hotspots;
    }

    ParsedHotspot parse_single_hotspot() {
        ParsedHotspot h;
        skip_whitespace();
        expect('{');
        while (peek() != '}') {
            std::string key = parse_string();
            skip_whitespace();
            expect(':');

            if (key == "name") {
                h.name = parse_string();
            } else if (key == "file") {
                h.file = parse_string();
            } else if (key == "line") {
                h.line = static_cast<int>(parse_int());
            } else if (key == "time_ns") {
                h.time_ns = parse_uint();
            } else if (key == "time_ms") {
                h.time_ms = parse_double();
            } else if (key == "percent") {
                h.percent = parse_double();
            } else if (key == "operations") {
                h.operations = parse_uint();
            } else {
                skip_value();
            }

            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect('}');
        return h;
    }

    std::vector<ParsedOperation> parse_operations() {
        std::vector<ParsedOperation> ops;
        skip_whitespace();
        expect('[');
        skip_whitespace();
        while (peek() != ']') {
            ops.push_back(parse_single_operation());
            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect(']');
        return ops;
    }

    ParsedOperation parse_single_operation() {
        ParsedOperation op;
        skip_whitespace();
        expect('{');
        while (peek() != '}') {
            std::string key = parse_string();
            skip_whitespace();
            expect(':');

            if (key == "type") {
                op.type = parse_string();
            } else if (key == "count") {
                op.count = parse_uint();
            } else if (key == "percent") {
                op.percent = parse_double();
            } else {
                skip_value();
            }

            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect('}');
        return op;
    }

    std::vector<analysis::OptimizationPattern> parse_suggestions() {
        std::vector<analysis::OptimizationPattern> suggestions;
        skip_whitespace();
        expect('[');
        skip_whitespace();
        while (peek() != ']') {
            suggestions.push_back(parse_single_suggestion());
            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect(']');
        return suggestions;
    }

    analysis::Severity parse_severity(const std::string& s) {
        if (s == "HIGH") return analysis::Severity::HIGH;
        if (s == "MEDIUM") return analysis::Severity::MEDIUM;
        return analysis::Severity::LOW;
    }

    analysis::PatternCategory parse_category(const std::string& s) {
        if (s == "Algorithmic") return analysis::PatternCategory::ALGORITHMIC;
        if (s == "Memory Access") return analysis::PatternCategory::MEMORY_ACCESS;
        if (s == "Arithmetic") return analysis::PatternCategory::ARITHMETIC;
        if (s == "Vectorization") return analysis::PatternCategory::VECTORIZATION;
        if (s == "Parallelization") return analysis::PatternCategory::PARALLELIZATION;
        if (s == "Precision") return analysis::PatternCategory::PRECISION;
        return analysis::PatternCategory::OTHER;
    }

    analysis::OptimizationPattern parse_single_suggestion() {
        analysis::OptimizationPattern pat{};
        skip_whitespace();
        expect('{');
        while (peek() != '}') {
            std::string key = parse_string();
            skip_whitespace();
            expect(':');

            if (key == "severity") {
                pat.severity = parse_severity(parse_string());
            } else if (key == "title") {
                pat.pattern_name = parse_string();
            } else if (key == "category") {
                pat.category = parse_category(parse_string());
            } else if (key == "location") {
                // Nested object
                skip_whitespace();
                expect('{');
                while (peek() != '}') {
                    std::string loc_key = parse_string();
                    skip_whitespace();
                    expect(':');
                    if (loc_key == "file") {
                        pat.location.set_file(parse_string());
                    } else if (loc_key == "line") {
                        pat.location.line = static_cast<int>(parse_int());
                    } else if (loc_key == "function") {
                        pat.location.set_function(parse_string());
                    } else {
                        skip_value();
                    }
                    skip_whitespace();
                    if (peek() == ',') advance();
                }
                expect('}');
            } else if (key == "speedup") {
                parse_string(); // skip display string
            } else if (key == "speedup_min") {
                pat.estimated_speedup_min = static_cast<float>(parse_double());
            } else if (key == "speedup_max") {
                pat.estimated_speedup_max = static_cast<float>(parse_double());
            } else if (key == "description") {
                pat.description = parse_string();
            } else if (key == "why_slow") {
                pat.why_slow = parse_string();
            } else if (key == "rationale") {
                pat.rationale = parse_string();
            } else if (key == "current_code") {
                pat.current_code = parse_string();
            } else if (key == "optimized_code") {
                pat.optimized_code = parse_string();
            } else if (key == "time_ns") {
                pat.time_ns = parse_uint();
            } else if (key == "time_percent") {
                pat.percentage_of_total = parse_double();
            } else if (key == "requirements") {
                pat.requirements = parse_string_array();
            } else if (key == "references") {
                pat.references = parse_string_array();
            } else if (key == "line_start") {
                pat.line_start = static_cast<int>(parse_int());
            } else if (key == "line_end") {
                pat.line_end = static_cast<int>(parse_int());
            } else if (key == "patchable") {
                skip_whitespace();
                if (peek() == 't') { pos_ += 4; pat.patchable = true; }
                else { pos_ += 5; pat.patchable = false; }
            } else {
                skip_value();
            }

            skip_whitespace();
            if (peek() == ',') advance();
        }
        expect('}');
        return pat;
    }
};

} // anonymous namespace

ParsedDashboard parse_dashboard_json(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open JSON file: " + filepath);
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();

    JsonParser parser(content);
    return parser.parse();
}

} // namespace json
} // namespace optiweave
