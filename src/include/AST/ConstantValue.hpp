#ifndef AST_CONSTANT_VALUE_NODE_H
#define AST_CONSTANT_VALUE_NODE_H

#include "AST/PType.hpp"
#include "AST/constant.hpp"
#include "AST/expression.hpp"
#include "visitor/AstNodeVisitor.hpp"

#include <memory>

class ConstantValueNode final : public ExpressionNode {
  private:
    std::unique_ptr<Constant> m_constant_ptr;

  public:
    ~ConstantValueNode() = default;
    ConstantValueNode(const uint32_t line, const uint32_t col,
                      Constant *const p_constant)
        : ExpressionNode{line, col}, m_constant_ptr(p_constant) {}

    const PType *getTypePtr() const { return m_constant_ptr->getTypePtr(); }
    const PTypeSharedPtr &getTypeSharedPtr() const {
        return m_constant_ptr->getTypeSharedPtr();
    }

    const char *getConstantValueCString() const {
        return m_constant_ptr->getConstantValueCString();
    }

    const Constant *getConstantPtr() const { return m_constant_ptr.get(); }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }
};

#endif
