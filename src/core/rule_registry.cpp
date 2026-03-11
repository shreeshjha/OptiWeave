#include <optiweave/core/rule_registry.hpp>
#include <algorithm>

namespace optiweave {
namespace core {

// --- FixRuleRegistry ---

const std::vector<FixRule*> FixRuleRegistry::empty_;

FixRuleRegistry& FixRuleRegistry::instance() {
    static FixRuleRegistry inst;
    return inst;
}

void FixRuleRegistry::register_rule(std::unique_ptr<FixRule> rule) {
    auto* ptr = rule.get();
    for (auto kind : ptr->subscribed_nodes()) {
        by_kind_[static_cast<int>(kind)].push_back(ptr);
    }
    rules_.push_back(std::move(rule));
}

const std::vector<FixRule*>& FixRuleRegistry::rules_for(ASTNodeKind kind) const {
    auto it = by_kind_.find(static_cast<int>(kind));
    if (it != by_kind_.end()) return it->second;
    return empty_;
}

FixRule* FixRuleRegistry::find_rule(const std::string& id) const {
    for (auto& r : rules_) {
        if (r->id() == id) return r.get();
    }
    return nullptr;
}

// --- PatchRuleRegistry ---

PatchRuleRegistry& PatchRuleRegistry::instance() {
    static PatchRuleRegistry inst;
    return inst;
}

void PatchRuleRegistry::register_rule(std::unique_ptr<PatchRule> rule) {
    rules_.push_back(std::move(rule));
}

std::vector<PatchRule*> PatchRuleRegistry::rules_for_pattern(std::string_view pattern_name) const {
    std::vector<PatchRule*> result;
    for (auto& r : rules_) {
        for (auto sv : r->handled_patterns()) {
            if (sv == pattern_name) {
                result.push_back(r.get());
                break;
            }
        }
    }
    return result;
}

PatchRule* PatchRuleRegistry::find_rule(const std::string& id) const {
    for (auto& r : rules_) {
        if (r->id() == id) return r.get();
    }
    return nullptr;
}

// --- register_builtin_rules() ---
// Forward declarations for all rule registration functions.
// Each rule .cpp file defines a register function.

// Fix rules (Phase 2 - existing)
void register_unsigned_wraparound_rule(FixRuleRegistry& reg);
void register_signed_negation_rule(FixRuleRegistry& reg);
void register_signed_left_shift_rule(FixRuleRegistry& reg);
void register_uninitialized_var_rule(FixRuleRegistry& reg);
void register_unused_var_rule(FixRuleRegistry& reg);
void register_fp_equality_rule(FixRuleRegistry& reg);

// Fix rules (Phase 5 - new)
void register_division_by_zero_rule(FixRuleRegistry& reg);
void register_null_deref_guard_rule(FixRuleRegistry& reg);
void register_string_overflow_rule(FixRuleRegistry& reg);
void register_implicit_fallthrough_rule(FixRuleRegistry& reg);
void register_sizeof_pointer_rule(FixRuleRegistry& reg);
void register_integer_truncation_rule(FixRuleRegistry& reg);
void register_dangling_else_rule(FixRuleRegistry& reg);

// Patch rules (Phase 3 - existing)
void register_division_reciprocal_rule(PatchRuleRegistry& reg);
void register_vectorization_pragma_rule(PatchRuleRegistry& reg);
void register_advisory_comment_rule(PatchRuleRegistry& reg);

// Patch rules (Phase 6 - new)
void register_loop_interchange_rule(PatchRuleRegistry& reg);
void register_strength_reduction_rule(PatchRuleRegistry& reg);
void register_loop_unroll_hint_rule(PatchRuleRegistry& reg);
void register_prefetch_hint_rule(PatchRuleRegistry& reg);
void register_restrict_qualifier_rule(PatchRuleRegistry& reg);

void register_builtin_rules() {
    auto& fix_reg = FixRuleRegistry::instance();
    auto& patch_reg = PatchRuleRegistry::instance();

    // Existing fix rules
    register_unsigned_wraparound_rule(fix_reg);
    register_signed_negation_rule(fix_reg);
    register_signed_left_shift_rule(fix_reg);
    register_uninitialized_var_rule(fix_reg);
    register_unused_var_rule(fix_reg);
    register_fp_equality_rule(fix_reg);

    // New fix rules
    register_division_by_zero_rule(fix_reg);
    register_null_deref_guard_rule(fix_reg);
    register_string_overflow_rule(fix_reg);
    register_implicit_fallthrough_rule(fix_reg);
    register_sizeof_pointer_rule(fix_reg);
    register_integer_truncation_rule(fix_reg);
    register_dangling_else_rule(fix_reg);

    // Existing patch rules
    register_division_reciprocal_rule(patch_reg);
    register_vectorization_pragma_rule(patch_reg);
    register_advisory_comment_rule(patch_reg);

    // New patch rules
    register_loop_interchange_rule(patch_reg);
    register_strength_reduction_rule(patch_reg);
    register_loop_unroll_hint_rule(patch_reg);
    register_prefetch_hint_rule(patch_reg);
    register_restrict_qualifier_rule(patch_reg);
}

} // namespace core
} // namespace optiweave
