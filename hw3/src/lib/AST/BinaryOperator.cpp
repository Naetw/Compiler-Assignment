#include "AST/BinaryOperator.hpp"

void BinaryOperatorNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(p_visitor); };

    visit_ast_node(m_left_operand);
    visit_ast_node(m_right_operand);
}
