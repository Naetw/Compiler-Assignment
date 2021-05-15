#ifndef AST_DECL_NODE_H
#define AST_DECL_NODE_H

#include "AST/ast.hpp"
#include "AST/utils.hpp"
#include "AST/variable.hpp"
#include "visitor/AstNodeVisitor.hpp"

#include <memory>
#include <vector>

class DeclNode final : public AstNode {
  public:
    using VarNodes = std::vector<std::unique_ptr<VariableNode>>;

  private:
    VarNodes m_var_nodes;

  private:
    void init(const std::vector<IdInfo> *const p_ids,
              const PTypeSharedPtr &p_type,
              ConstantValueNode *const p_constant);

  public:
    ~DeclNode() = default;

    // variable declaration
    DeclNode(const uint32_t line, const uint32_t col
             /* TODO: identifiers, type */)
        : AstNode{line, col} {}

    // constant variable declaration
    DeclNode(const uint32_t line, const uint32_t col,
             const std::vector<IdInfo> *const p_ids,
             ConstantValueNode *const p_constant)
        : AstNode{line, col} {
        init(p_ids, p_constant->getTypeSharedPtr(), p_constant);
    }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }
    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
