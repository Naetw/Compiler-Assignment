#include "AST/assignment.hpp"

void AssignmentNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    m_lvalue->accept(p_visitor);
    m_expr->accept(p_visitor);
}
