#include "sema/SymbolTable.hpp"

#include <cassert>
#include <cstdio>
#include <utility> // for piecewise_construct

/******************************************************************************
 * > Attribute
 ******************************************************************************/
Attribute::Attribute(const Constant *p_constant)
    : type(Tag::kConstantValue), constant_value_ptr(p_constant) {}

Attribute::Attribute(const FunctionNode::Decls *p_parameters)
    : type(Tag::kParameterDecls), parameters_ptr(p_parameters) {}

const Constant *Attribute::constant() const {
    if (type != Tag::kConstantValue) {
        assert(false && "Shouldn't reach here!");
        return nullptr;
    }

    return constant_value_ptr;
}

const FunctionNode::Decls *Attribute::parameters() const {
    if (type != Tag::kParameterDecls) {
        assert(false && "Shouldn't reach here!");
        return nullptr;
    }

    return parameters_ptr;
}

/******************************************************************************
 * > SymbolEntry
 ******************************************************************************/
SymbolEntry::SymbolEntry(const std::string &p_name,
                         const SymbolEntry::KindEnum kind, const size_t level,
                         const PType *p_type, const Constant *p_constant,
                         const SymbolEntry *p_prev)
    : name(p_name), kind(kind), level(level), type(p_type),
      attribute(p_constant), prev_entry(p_prev), has_error(false) {}

SymbolEntry::SymbolEntry(const std::string &p_name,
                         const SymbolEntry::KindEnum kind, const size_t level,
                         const PType *p_type,
                         const FunctionNode::Decls *p_parameters,
                         const SymbolEntry *p_prev)
    : name(p_name), kind(kind), level(level), type(p_type),
      attribute(p_parameters), prev_entry(p_prev), has_error(false) {}

const std::string &SymbolEntry::getName() const { return name; }

const char *SymbolEntry::getNameCString() const { return name.c_str(); }

const SymbolEntry::KindEnum SymbolEntry::getKind() const { return kind; }

const size_t SymbolEntry::getLevel() const { return level; }

const PType *SymbolEntry::getTypePtr() const { return type; }

const Attribute &SymbolEntry::getAttribute() const { return attribute; }

const SymbolEntry *SymbolEntry::getPrevEntry() const { return prev_entry; }

bool SymbolEntry::hasError() const { return has_error; }

void SymbolEntry::setError() { has_error = true; }

/******************************************************************************
 * > SymbolTable
 ******************************************************************************/
const std::vector<std::unique_ptr<SymbolEntry>> &
SymbolTable::getEntries() const {
    return entries;
}

// return (existence, old_entry) pair
static std::pair<bool, const SymbolEntry *>
checkExistence(std::map<std::string, const SymbolEntry *> &hash_entries,
               const std::string &name, const size_t current_level) {
    auto search_result = hash_entries.find(name);
    const SymbolEntry *old_entry = nullptr;

    if (search_result != hash_entries.end()) {
        old_entry = search_result->second;

        if (old_entry->getLevel() == current_level ||
            old_entry->getKind() == SymbolEntry::KindEnum::kLoopVarKind) {
            return std::make_pair(true, old_entry);
        }
    }

    return std::make_pair(false, old_entry);
}

SymbolEntry *SymbolTable::addSymbol(const std::string &p_name,
                                    const SymbolEntry::KindEnum kind,
                                    const size_t level, const PType *p_type,
                                    const Constant *p_constant,
                                    const SymbolEntry *p_prev) {
    entries.emplace_back(
        new SymbolEntry(p_name, kind, level, p_type, p_constant, p_prev));
    return const_cast<SymbolEntry *>(entries.back().get());
}

SymbolEntry *SymbolTable::addSymbol(const std::string &p_name,
                                    const SymbolEntry::KindEnum kind,
                                    const size_t level, const PType *p_type,
                                    const FunctionNode::Decls *p_parameters,
                                    const SymbolEntry *p_prev) {
    entries.emplace_back(
        new SymbolEntry(p_name, kind, level, p_type, p_parameters, p_prev));
    return const_cast<SymbolEntry *>(entries.back().get());
}

/******************************************************************************
 * > SymbolManager
 ******************************************************************************/
SymbolManager::SymbolManager(const bool opt_dmp)
    : in_use_tables(1, nullptr), current_table(nullptr), current_level(0u),
      opt_dmp(opt_dmp) {}

SymbolManager::~SymbolManager() {
    for (auto table : in_use_tables) {
        delete table;
    }

    for (auto table : popped_tables) {
        delete table;
    }
}

void SymbolManager::pushGlobalScope() {
    pushScope();
    current_level--;
}

void SymbolManager::pushScope() {
    SymbolTable *new_table = new SymbolTable();

    assert(new_table != NULL && "fail to allocate memory");

    in_use_tables.emplace_back(new_table);
    current_table = new_table;
    current_level++;
}

static void dumpSymbolTable(const SymbolTable *table) {
    static const char *kKindStrings[] = {"program",  "function", "parameter",
                                         "variable", "loop_var", "constant"};

    std::printf("=========================================================="
                "====================================================\n");
    std::printf("%-33s%-11s%-11s%-17s%-11s\n", "Name", "Kind", "Level", "Type",
                "Attribute");
    std::printf("----------------------------------------------------------"
                "----------------------------------------------------\n");

    for (const auto &entry_ptr : table->getEntries()) {
        std::printf("%-33s", entry_ptr->getNameCString());
        std::printf("%-11s",
                    kKindStrings[static_cast<uint32_t>(entry_ptr->getKind())]);
        std::printf("%lu%-10s", entry_ptr->getLevel(),
                    (entry_ptr->getLevel()) ? "(local)" : "(global)");
        std::printf("%-17s", entry_ptr->getTypePtr()->getPTypeCString());

        std::string type_string;
        const char *attr;

        if (entry_ptr->getKind() == SymbolEntry::KindEnum::kFunctionKind) {
            const FunctionNode::Decls *parameters =
                entry_ptr->getAttribute().parameters();
            if (parameters) {
                type_string =
                    FunctionNode::getParametersTypeString(*parameters);
                attr = type_string.c_str();
            } else {
                attr = "";
            }
        } else {
            const Constant *constant = entry_ptr->getAttribute().constant();
            if (constant) {
                attr = constant->getConstantValueCString();
            } else {
                attr = "";
            }
        }
        std::printf("%-11s\n", attr);
    }

    std::printf("----------------------------------------------------------"
                "----------------------------------------------------\n");
}

void SymbolManager::prevScope() {
    assert(
        current_table &&
        "If happens, it means that the uses of prevScope() are more than the "
        "ones of pushScope().");

    SymbolTable *ct = current_table;

    removeSymbolsFromHashTable(ct);
    in_use_tables.pop_back();
    current_table = in_use_tables.back();
    popped_tables.emplace_back(ct);
    current_level--;
}

void SymbolManager::popGlobalScope() {
    current_level++;
    popScope();
}

void SymbolManager::popScope() {
    if (!current_table) {
        assert(false && "Shouldn't popScope() without pushing any scope");
        return;
    }

    if (opt_dmp) {
        dumpSymbolTable(current_table);
    }
    prevScope();
}

SymbolEntry *SymbolManager::addSymbol(const std::string &p_name,
                                      const SymbolEntry::KindEnum kind,
                                      const PType *p_type,
                                      const Constant *p_constant) {
    auto result_pair = checkExistence(hash_entries, p_name, current_level);

    if (result_pair.first) {
        return nullptr;
    }

    auto *new_entry = current_table->addSymbol(
        p_name, kind, current_level, p_type, p_constant, result_pair.second);

    if (result_pair.second) {
        // replace symbol in outer scope
        hash_entries[p_name] = new_entry;
    } else {
        hash_entries.emplace(std::piecewise_construct,
                             std::forward_as_tuple(p_name),
                             std::forward_as_tuple(new_entry));
    }

    return const_cast<SymbolEntry *>(new_entry);
}

SymbolEntry *SymbolManager::addSymbol(const std::string &p_name,
                                      const SymbolEntry::KindEnum kind,
                                      const PType *p_type,
                                      const FunctionNode::Decls *p_parameters) {
    auto result_pair = checkExistence(hash_entries, p_name, current_level);

    if (result_pair.first) {
        return nullptr;
    }

    auto *new_entry = current_table->addSymbol(
        p_name, kind, current_level, p_type, p_parameters, result_pair.second);

    if (result_pair.second) {
        // replace symbol in outer scope
        hash_entries[p_name] = new_entry;
    } else {
        hash_entries.emplace(std::piecewise_construct,
                             std::forward_as_tuple(p_name),
                             std::forward_as_tuple(new_entry));
    }

    return const_cast<SymbolEntry *>(new_entry);
}

const SymbolEntry *SymbolManager::lookup(const std::string &p_name) const {
    auto search_result = hash_entries.find(p_name);

    if (search_result != hash_entries.end()) {
        return search_result->second;
    }
    return nullptr;
}

const SymbolTable *SymbolManager::getCurrentTable() const {
    return current_table;
}

void SymbolManager::reconstructHashTableFromSymbolTable(
    const SymbolTable *table) {
    if (!table) {
        return;
    }

    const auto &entries = table->getEntries();

    for (const auto &entry_ptr : entries) {
        auto result_pair = checkExistence(hash_entries, entry_ptr->getName(),
                                          entry_ptr->getLevel());

        if (result_pair.second) {
            hash_entries[entry_ptr->getName()] = entry_ptr.get();
        } else {
            hash_entries.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(entry_ptr->getName()),
                                 std::forward_as_tuple(entry_ptr.get()));
        }
    }
}

void SymbolManager::removeSymbolsFromHashTable(const SymbolTable *table) {
    if (!table) {
        return;
    }

    const auto &entries = table->getEntries();

    for (const auto &entry_ptr : entries) {
        auto search_result = hash_entries.find(entry_ptr->getName());
        assert(search_result != hash_entries.end() &&
               "CANNOT remove the symbol which doesn't exist");

        if (entry_ptr->getPrevEntry()) {
            hash_entries[entry_ptr->getName()] = entry_ptr->getPrevEntry();
        } else {
            hash_entries.erase(search_result);
        }
    }
}
