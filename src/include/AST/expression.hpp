#ifndef AST_EXPRESSION_NODE_H
#define AST_EXPRESSION_NODE_H

#include "AST/ast.hpp"
#include "AST/PType.hpp"

#include <memory>

class ExpressionNode : public AstNode {
  protected:
    // for carrying type of result of an expression
      std::unique_ptr<PType> m_type;

  public:
    ~ExpressionNode() = default;
    ExpressionNode(const uint32_t line, const uint32_t col)
        : AstNode{line, col} {}

    const PType *getInferredType() const { return m_type.get(); }
    void setInferredType(PType *p_type) { m_type.reset(p_type); }
};

#endif
