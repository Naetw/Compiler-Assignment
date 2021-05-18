#include "AST/if.hpp"

void IfNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    m_condition->accept(p_visitor);  
    m_body->accept(p_visitor);
    if (m_else_body) {
        m_else_body->accept(p_visitor);
    }
}
