#ifndef AST_BINARY_OPERATOR_NODE_H
#define AST_BINARY_OPERATOR_NODE_H

#include "AST/expression.hpp"
#include "AST/operator.hpp"
#include "visitor/AstNodeVisitor.hpp"

#include <memory>

class BinaryOperatorNode final : public ExpressionNode {
  private:
    Operator m_op;
    std::unique_ptr<ExpressionNode> m_left_operand;
    std::unique_ptr<ExpressionNode> m_right_operand;

  public:
    ~BinaryOperatorNode() = default;
    BinaryOperatorNode(const uint32_t line, const uint32_t col, Operator op,
                       ExpressionNode *p_left_operand,
                       ExpressionNode *p_right_operand)
        : ExpressionNode{line, col}, m_op(op), m_left_operand(p_left_operand),
          m_right_operand(p_right_operand) {}

    Operator getOp() const { return m_op; }

    const char *getOpCString() const {
        return kOpString[static_cast<size_t>(m_op)];
    }

    const ExpressionNode &getLeftOperand() const { return *m_left_operand.get(); }
    const ExpressionNode &getRightOperand() const { return *m_right_operand.get(); }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }
    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
