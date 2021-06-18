#include "sema/SymbolTable.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>

// ===========================================
// > Attribute
// ===========================================
const Constant *Attribute::constant() const {
    if (m_type != Tag::kConstantValue) {
        assert(false && "Try to extract constant from an attribute object that "
                        "is not for constant");
        return nullptr;
    }
    return m_constant_value_ptr;
}

const FunctionNode::DeclNodes *Attribute::parameters() const {
    if (m_type != Tag::kParameterDeclNodes) {
        assert(false && "Try to extract parameters from an attribute object"
                        "that is not for parameters");
        return nullptr;
    }
    return m_parameters_ptr;
}

// ===========================================
// > SymbolTable
// ===========================================
SymbolEntry *SymbolTable::addSymbol(const std::string &p_name,
                                    const SymbolEntry::KindEnum kind,
                                    const size_t level,
                                    const PType *const p_type,
                                    const Constant *const p_constant) {
    m_entries.emplace_back(
        new SymbolEntry(p_name, kind, level, p_type, p_constant));
    return m_entries.back().get();
}

SymbolEntry *
SymbolTable::addSymbol(const std::string &p_name,
                       const SymbolEntry::KindEnum kind, const size_t level,
                       const PType *const p_type,
                       const FunctionNode::DeclNodes *const p_parameters) {
    m_entries.emplace_back(
        new SymbolEntry(p_name, kind, level, p_type, p_parameters));
    return m_entries.back().get();
}

// ===========================================
// > SymbolManager
// ===========================================
void SymbolManager::pushGlobalScope() {
    pushScope();

    m_current_level--; // since the level of global is 0
}

void SymbolManager::pushScope() {
    SymbolTable *new_table = new SymbolTable();

    assert(new_table != nullptr && "Fail to allocate memory for SymbolTable");

    m_in_use_tables.emplace_back(new_table);
    m_current_table = new_table;
    m_current_level++;
}

void SymbolManager::popGlobalScope() {
    m_current_level++; // prevent level from 0 to max value of size_t
    popScope();
}

static void dumpSymbolTable(const SymbolTable *const table) {
    static const char *kKindStrings[] = {"program",  "function", "parameter",
                                         "variable", "loop_var", "constant"};

    std::printf("=========================================================="
                "====================================================\n");
    std::printf("%-33s%-11s%-11s%-17s%-11s\n", "Name", "Kind", "Level", "Type",
                "Attribute");
    std::printf("----------------------------------------------------------"
                "----------------------------------------------------\n");

    std::string type_string;
    auto construct_attr_string = [&type_string](const auto &p_entry_ptr) {
        if (p_entry_ptr->getKind() == SymbolEntry::KindEnum::kFunctionKind) {
            const FunctionNode::DeclNodes *const parameters_ptr =
                p_entry_ptr->getAttribute().parameters();
            type_string =
                FunctionNode::getParametersTypeString(*parameters_ptr);
            return type_string.c_str();
        } else {
            const Constant *const constant =
                p_entry_ptr->getAttribute().constant();
            if (constant) {
                return constant->getConstantValueCString();
            } else {
                return "";
            }
        }
    };

    auto dump_entry = [&](const auto &p_entry_ptr) {
        std::printf("%-33s", p_entry_ptr->getNameCString());
        std::printf("%-11s",
                    kKindStrings[static_cast<size_t>(p_entry_ptr->getKind())]);
        std::printf("%lu%-10s", p_entry_ptr->getLevel(),
                    (p_entry_ptr->getLevel() != 0) ? "(local)" : "(global)");
        std::printf("%-17s", p_entry_ptr->getTypePtr()->getPTypeCString());
        std::printf("%-11s\n", construct_attr_string(p_entry_ptr));
    };

    for_each(table->getEntries().begin(), table->getEntries().end(),
             dump_entry);

    std::printf("----------------------------------------------------------"
                "----------------------------------------------------\n");
}

void SymbolManager::reconstructHashTableFromSymbolTable(
    const SymbolTable *const p_table) const {
    if (!p_table) {
        return;
    }

    auto construct_entry_on_hash_map = [&](const auto &p_entry_ptr) {
        auto existence_pair =
            checkExistence(p_entry_ptr->getName(), p_entry_ptr->getLevel());

        // No need to care existence_pair.first since it's for semantic check.
        // In the reconstruction, the whole symbol tables have been constructed
        // before.

        if (existence_pair.second) {
            m_hidden_entries[p_entry_ptr->getName()].push(
                existence_pair.second);
            m_hash_entries[p_entry_ptr->getName()] = p_entry_ptr.get();
        } else {
            m_hash_entries.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(p_entry_ptr->getName()),
                std::forward_as_tuple(p_entry_ptr.get()));
        }
    };

    for_each(p_table->getEntries().begin(), p_table->getEntries().end(),
             construct_entry_on_hash_map);
}

void SymbolManager::removeSymbolsFromHashTable(
    const SymbolTable *const p_table) const {
    if (!p_table) {
        return;
    }

    auto remove_entry_from_hash_map = [&](const auto &p_entry_ptr) {
        auto hash_search_result = m_hash_entries.find(p_entry_ptr->getName());
        assert(hash_search_result != m_hash_entries.end() &&
               "CANNOT remove the symbol that doesn't exist");

        auto stack_search_result =
            m_hidden_entries.find(p_entry_ptr->getName());
        if (stack_search_result != m_hidden_entries.end()) {
            auto &prev_entry_stack = stack_search_result->second;
            m_hash_entries[p_entry_ptr->getName()] = prev_entry_stack.top();
            prev_entry_stack.pop();
            if (prev_entry_stack.empty()) {
                m_hidden_entries.erase(stack_search_result);
            }
        } else {
            m_hash_entries.erase(hash_search_result);
        }
    };

    for_each(p_table->getEntries().begin(), p_table->getEntries().end(),
             remove_entry_from_hash_map);
}

void SymbolManager::prevScope() {
    assert(m_current_table &&
           "If happens, it means that the uses of popScope() are more than the"
           "ones of pushScope()");

    removeSymbolsFromHashTable(m_current_table);

    SymbolTable *prev_cur_table = m_current_table;
    m_in_use_tables.back().release();
    m_in_use_tables.pop_back();
    m_current_table = m_in_use_tables.back().get();
    m_popped_tables.emplace_back(prev_cur_table);
    m_current_level--;
}

void SymbolManager::popScope() {
    if (!m_current_table) {
        assert(false && "Shouldn't popScope() without pushing any scope");
        return;
    }
    if (m_opt_dmp) {
        dumpSymbolTable(m_current_table);
    }
    prevScope();
}

std::pair<bool, SymbolEntry *>
SymbolManager::checkExistence(const std::string &p_name,
                              const size_t current_level) const {
    auto search_result = m_hash_entries.find(p_name);

    if (search_result != m_hash_entries.end()) {
        SymbolEntry *old_entry = search_result->second;

        if (old_entry->getLevel() == current_level ||
            old_entry->getKind() == SymbolEntry::KindEnum::kLoopVarKind) {
            return std::make_pair(true, old_entry);
        } else {
            return std::make_pair(false, old_entry);
        }
    }

    return std::make_pair(false, nullptr);
}

template <typename AttributeType>
SymbolEntry *
genericAddSymbol(SymbolManager &p_manager, const std::string &p_name,
                 const SymbolEntry::KindEnum kind, const PType *const p_type,
                 const AttributeType *const p_attribute) {
    auto existence_pair =
        p_manager.checkExistence(p_name, p_manager.getCurrentLevel());

    if (existence_pair.first) {
        return nullptr;
    }

    auto *new_entry = p_manager.m_current_table->addSymbol(
        p_name, kind, p_manager.m_current_level, p_type, p_attribute);

    if (existence_pair.second) {
        // store the old entry
        p_manager.m_hidden_entries[p_name].push(existence_pair.second);

        // replace symbol in outer scope
        p_manager.m_hash_entries[p_name] = new_entry;
    } else {
        p_manager.m_hash_entries.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(p_name),
                                         std::forward_as_tuple(new_entry));
    }

    return new_entry;
}

SymbolEntry *SymbolManager::addSymbol(const std::string &p_name,
                                      const SymbolEntry::KindEnum kind,
                                      const PType *const p_type,
                                      const Constant *const p_constant) {
    return genericAddSymbol<Constant>(*this, p_name, kind, p_type, p_constant);
}

SymbolEntry *
SymbolManager::addSymbol(const std::string &p_name,
                         const SymbolEntry::KindEnum kind,
                         const PType *const p_type,
                         const FunctionNode::DeclNodes *const p_parameters) {
    return genericAddSymbol<FunctionNode::DeclNodes>(*this, p_name, kind,
                                                     p_type, p_parameters);
}

const SymbolEntry *SymbolManager::lookup(const std::string &p_name) const {
    auto search_result = m_hash_entries.find(p_name);

    if (search_result != m_hash_entries.end()) {
        return search_result->second;
    }
    return nullptr;
}
