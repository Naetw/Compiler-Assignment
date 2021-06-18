#ifndef AST_READ_NODE_H
#define AST_READ_NODE_H

#include "AST/ast.hpp"
#include "AST/VariableReference.hpp"

#include <memory>

class ReadNode final : public AstNode {
  private:
    std::unique_ptr<VariableReferenceNode> m_target;

  public:
    ~ReadNode() = default;
    ReadNode(const uint32_t line, const uint32_t col,
             VariableReferenceNode *p_target)
        : AstNode{line, col}, m_target(p_target){}

    const VariableReferenceNode &getTarget() const { return *m_target.get(); }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }
    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
