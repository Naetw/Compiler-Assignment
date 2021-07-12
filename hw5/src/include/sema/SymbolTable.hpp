#ifndef SEMA_SYMBOL_TABLE_H
#define SEMA_SYMBOL_TABLE_H

#include "AST/PType.hpp"
#include "AST/function.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

/*
 * Conform to C++ Core Guidelines C.182
 */
class Attribute {
  private:
    enum class Tag { kConstantValue, kParameterDeclNodes };
    Tag m_type;

    union {
        // raw pointer, does not own the object
        const Constant *m_constant_value_ptr;
        const FunctionNode::DeclNodes *m_parameters_ptr;
    };

  public:
    ~Attribute() = default;

    Attribute(const Constant *p_constant)
        : m_type(Tag::kConstantValue), m_constant_value_ptr(p_constant) {}

    Attribute(const FunctionNode::DeclNodes *p_parameters)
        : m_type(Tag::kParameterDeclNodes), m_parameters_ptr(p_parameters) {}

    const Constant *constant() const;
    const FunctionNode::DeclNodes *parameters() const;
};

class SymbolEntry {
  public:
    enum class KindEnum : uint8_t {
        kProgramKind,
        kFunctionKind,
        kParameterKind,
        kVariableKind,
        kLoopVarKind,
        kConstantKind
    };

  private:
    const std::string &m_name;
    KindEnum m_kind;
    size_t m_level;
    const PType *m_p_type;
    Attribute m_attribute;

  public:
    ~SymbolEntry() = default;

    SymbolEntry(const std::string &p_name, const KindEnum kind,
                const size_t level, const PType *const p_type,
                const Constant *const p_constant)
        : m_name(p_name), m_kind(kind), m_level(level), m_p_type(p_type),
          m_attribute(p_constant) {}

    SymbolEntry(const std::string &p_name, const KindEnum kind,
                const size_t level, const PType *const p_type,
                const FunctionNode::DeclNodes *const p_parameters)
        : m_name(p_name), m_kind(kind), m_level(level), m_p_type(p_type),
          m_attribute(p_parameters) {}

    const std::string &getName() const { return m_name; };
    const char *getNameCString() const { return m_name.c_str(); };

    const KindEnum getKind() const { return m_kind; };

    const size_t getLevel() const { return m_level; };

    const PType *getTypePtr() const { return m_p_type; };

    const Attribute &getAttribute() const { return m_attribute; };
};

class SymbolTable {
  public:
    using Entries = std::vector<std::unique_ptr<SymbolEntry>>;

  private:
    // general info
    Entries m_entries;

  public:
    ~SymbolTable() = default;
    SymbolTable() = default;

    const Entries &getEntries() const { return m_entries; };

    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind, const size_t level,
                           const PType *const p_type,
                           const Constant *const p_constant);
    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind, const size_t level,
                           const PType *const p_type,
                           const FunctionNode::DeclNodes *const p_parameters);
};

class SymbolManager {
  public:
    using Tables = std::vector<std::unique_ptr<SymbolTable>>;
    using NameEntryMap = std::map<std::string, SymbolEntry *>;

  private:
    Tables m_in_use_tables;

    // hold tables for other visitors to use
    Tables m_popped_tables;

    mutable NameEntryMap m_hash_entries;
    mutable std::map<std::string, std::stack<SymbolEntry *>> m_hidden_entries;

    SymbolTable *m_current_table = nullptr;
    size_t m_current_level = 0;

    const bool m_opt_dmp;

  public:
    ~SymbolManager() = default;
    SymbolManager(const bool opt_dmp) : m_opt_dmp(opt_dmp) {
        // for resetting m_current_table back to nullptr
        m_in_use_tables.emplace_back(nullptr);
    }

    // the effective behavior that popScope performs
    void prevScope();

    // initial construction
    void pushGlobalScope();
    void pushScope();
    void popGlobalScope();
    void popScope();

    template <typename AttributeType>
    friend SymbolEntry *
    genericAddSymbol(SymbolManager &p_manager, const std::string &p_name,
                     const SymbolEntry::KindEnum kind,
                     const PType *const p_type,
                     const AttributeType *const p_attribute);

    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind,
                           const PType *const p_type,
                           const Constant *const p_constant);
    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind,
                           const PType *const p_type,
                           const FunctionNode::DeclNodes *const p_parameters);

    const SymbolEntry *lookup(const std::string &p_name) const;

    const SymbolTable *getCurrentTable() const { return m_current_table; }
    size_t getCurrentLevel() const { return m_current_level; }

    void
    reconstructHashTableFromSymbolTable(const SymbolTable *const p_table) const;
    void removeSymbolsFromHashTable(const SymbolTable *const p_table) const;

  private:
    std::pair<bool, SymbolEntry *>
    checkExistence(const std::string &p_name, const size_t current_level) const;
};

#endif
