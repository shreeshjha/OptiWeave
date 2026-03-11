#pragma once

#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/patch_rule.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace optiweave {
namespace core {

/// Registry for fix rules, indexed by AST node kind.
class FixRuleRegistry {
public:
    static FixRuleRegistry& instance();

    void register_rule(std::unique_ptr<FixRule> rule);

    /// Get all rules subscribed to a given AST node kind.
    const std::vector<FixRule*>& rules_for(ASTNodeKind kind) const;

    /// Get all registered rules.
    const std::vector<std::unique_ptr<FixRule>>& all_rules() const { return rules_; }

    /// Find rule by id.
    FixRule* find_rule(const std::string& id) const;

private:
    FixRuleRegistry() = default;
    std::vector<std::unique_ptr<FixRule>> rules_;
    std::unordered_map<int, std::vector<FixRule*>> by_kind_;
    static const std::vector<FixRule*> empty_;
};

/// Registry for patch rules, indexed by pattern name.
class PatchRuleRegistry {
public:
    static PatchRuleRegistry& instance();

    void register_rule(std::unique_ptr<PatchRule> rule);

    /// Get all rules that handle a given pattern name.
    std::vector<PatchRule*> rules_for_pattern(std::string_view pattern_name) const;

    /// Get all registered rules.
    const std::vector<std::unique_ptr<PatchRule>>& all_rules() const { return rules_; }

    /// Find rule by id.
    PatchRule* find_rule(const std::string& id) const;

private:
    PatchRuleRegistry() = default;
    std::vector<std::unique_ptr<PatchRule>> rules_;
};

/// Call once at startup to register all built-in fix and patch rules.
void register_builtin_rules();

} // namespace core
} // namespace optiweave
