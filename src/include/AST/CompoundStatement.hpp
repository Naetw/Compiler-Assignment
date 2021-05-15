#ifndef AST_COMPOUND_STATEMENT_NODE_H
#define AST_COMPOUND_STATEMENT_NODE_H

#include "AST/ast.hpp"
#include "AST/decl.hpp"

#include <vector>
#include <memory>

class CompoundStatementNode final : public AstNode {
  public:
    using DeclNodes = std::vector<std::unique_ptr<DeclNode>>;

  private:
    // TODO: statements
    DeclNodes m_decl_nodes;

  public:
    ~CompoundStatementNode() = default;
    CompoundStatementNode(const uint32_t line, const uint32_t col,
                          DeclNodes &p_decl_nodes
                          /* TODO: declarations, statements */)
        : AstNode{line, col}, m_decl_nodes(std::move(p_decl_nodes)) {}

    void accept(AstNodeVisitor &p_visitor) override {
        p_visitor.visit(*this);
    }
    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
