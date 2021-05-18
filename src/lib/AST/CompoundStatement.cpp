#include "AST/CompoundStatement.hpp"

#include <algorithm>

void CompoundStatementNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    auto visit_ast_node = [&](auto &ast_node) {
        ast_node->accept(p_visitor);
    };

    for_each(m_decl_nodes.begin(), m_decl_nodes.end(), visit_ast_node);
    for_each(m_stmt_nodes.begin(), m_stmt_nodes.end(), visit_ast_node);
}
