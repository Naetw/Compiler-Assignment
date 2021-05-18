#include "AST/print.hpp"

void PrintNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    m_target->accept(p_visitor);
}
