#include "AST/program.hpp"
#include "AST/AstDumper.hpp"
#include "AST/CompoundStatement.hpp"

#include <algorithm>

void ProgramNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    auto visit_decl_node = [&](auto &decl_node) {
        decl_node->accept(p_visitor);
    };
    for_each(m_decl_nodes.begin(), m_decl_nodes.end(), visit_decl_node);

    // TODO: functions

    m_body->accept(p_visitor);
}
