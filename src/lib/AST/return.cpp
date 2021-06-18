#include "AST/return.hpp"

void ReturnNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    m_ret_val->accept(p_visitor);
}
