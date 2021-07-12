#include "AST/variable.hpp"

void VariableNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    if (m_constant_value_node_ptr) {
        m_constant_value_node_ptr->accept(p_visitor);
    }
}
