#include "AST/for.hpp"

void ForNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    m_loop_var_decl->accept(p_visitor);
    m_init_stmt->accept(p_visitor);
    m_end_condition->accept(p_visitor);
    m_body->accept(p_visitor);
}
