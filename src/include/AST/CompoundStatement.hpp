#ifndef __AST_COMPOUND_STATEMENT_NODE_H
#define __AST_COMPOUND_STATEMENT_NODE_H

#include "AST/ast.hpp"
#include "AST/decl.hpp"

#include <memory>
#include <vector>

class SymbolTable;

class CompoundStatementNode : public AstNode {
  public:
    typedef std::vector<std::unique_ptr<DeclNode>> Decls;
    typedef std::vector<std::unique_ptr<AstNode>> Nodes;

    CompoundStatementNode(const uint32_t line, const uint32_t col,
                          Decls *p_var_decls, Nodes *p_statements);
    ~CompoundStatementNode() = default;

    void setSymbolTable(const SymbolTable *table);
    const SymbolTable *getSymbolTable() const;

    void accept(AstNodeVisitor &p_visitor) override;
    void visitChildNodes(AstNodeVisitor &p_visitor) override;

  private:
    Decls var_decls;
    Nodes statements;

    const SymbolTable *symbol_table;
};

#endif
