#include "AST/for.hpp"

#include <cassert>

const ConstantValueNode &ForNode::getLowerBound() const {
    const auto *const lower_ptr =
        dynamic_cast<const ConstantValueNode *>(&m_init_stmt->getExpr());

    assert(lower_ptr && "Shouldn't reach here since the syntax has "
                        "ensured that it will be a constant value");

    return *lower_ptr;
}

const ConstantValueNode &ForNode::getUpperBound() const {
    const auto *const upper_ptr =
        dynamic_cast<const ConstantValueNode *>(m_end_condition.get());

    assert(upper_ptr && "Shouldn't reach here since the syntax has "
                        "ensured that it will be a constant value");

    return *upper_ptr;
}

void ForNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    m_loop_var_decl->accept(p_visitor);
    m_init_stmt->accept(p_visitor);
    m_end_condition->accept(p_visitor);
    m_body->accept(p_visitor);
}
