#include "AST/CompoundStatement.hpp"

#include <algorithm>

void CompoundStatementNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    auto visit_decl_node = [&](auto &decl_node) {
        decl_node->accept(p_visitor);
    };
    for_each(m_decl_nodes.begin(), m_decl_nodes.end(), visit_decl_node);
}
