#ifndef __SEMA_SYMBOL_TABLE_H
#define __SEMA_SYMBOL_TABLE_H

#include "AST/PType.hpp"
#include "AST/function.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

/*
 * Conform to C++ Core Guidelines C.182
 */
class Attribute {
  public:
    Attribute(const Constant *p_constant);
    Attribute(const FunctionNode::Decls *p_parameters);
    ~Attribute() = default;

    const Constant *constant() const;
    const FunctionNode::Decls *parameters() const;

  private:
    enum class Tag { kConstantValue, kParameterDecls };
    Tag type;

    union {
        // raw pointer, does not own the object
        const Constant *constant_value_ptr;
        const FunctionNode::Decls *parameters_ptr;
    };
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

    SymbolEntry(const std::string &p_name, const KindEnum kind,
                const size_t level, const PType *p_type,
                const Constant *p_constant, const SymbolEntry *p_prev);
    SymbolEntry(const std::string &p_name, const KindEnum kind,
                const size_t level, const PType *p_type,
                const FunctionNode::Decls *p_parameters,
                const SymbolEntry *p_prev);
    ~SymbolEntry() = default;

    const std::string &getName() const;
    const char *getNameCString() const;
    const KindEnum getKind() const;
    const size_t getLevel() const;
    const PType *getTypePtr() const;
    const Attribute &getAttribute() const;
    const SymbolEntry *getPrevEntry() const;

    bool hasError() const;

    void setError();

  private:
    const std::string &name;
    const KindEnum kind;
    const size_t level;
    const PType *type;
    const Attribute attribute;

    // record the outer symbol
    const SymbolEntry *prev_entry;

    bool has_error;
};

class SymbolTable {
  public:
    SymbolTable() = default;
    ~SymbolTable() = default;

    const std::vector<std::unique_ptr<SymbolEntry>> &getEntries() const;

    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind, const size_t level,
                           const PType *p_type, const Constant *p_constant,
                           const SymbolEntry *p_prev);
    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind, const size_t level,
                           const PType *p_type,
                           const FunctionNode::Decls *p_parameters,
                           const SymbolEntry *p_prev);

  private:
    // general info
    std::vector<std::unique_ptr<SymbolEntry>> entries;
};

class SymbolManager {
  public:
    SymbolManager(const bool opt_dmp);
    ~SymbolManager();

    // the real behavior that popScope performs
    void prevScope();

    // initial construction
    void pushGlobalScope();
    void pushScope();
    void popGlobalScope();
    void popScope();

    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind,
                           const PType *p_type, const Constant *p_constant);
    SymbolEntry *addSymbol(const std::string &p_name,
                           const SymbolEntry::KindEnum kind,
                           const PType *p_type,
                           const FunctionNode::Decls *p_parameters);

    const SymbolEntry *lookup(const std::string &p_name) const;

    const SymbolTable *getCurrentTable() const;

    void reconstructHashTableFromSymbolTable(const SymbolTable *table);
    void removeSymbolsFromHashTable(const SymbolTable *table);

  private:
    std::vector<SymbolTable *> in_use_tables;

    // hold tables for other visitors to use
    std::vector<SymbolTable *> popped_tables;

    std::map<std::string, const SymbolEntry *> hash_entries;

    SymbolTable *current_table;
    size_t current_level;
    const bool opt_dmp;
};

#endif
