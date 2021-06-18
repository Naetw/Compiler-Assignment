#include "AST/for.hpp"
#include "visitor/AstNodeVisitor.hpp"

#include <cassert>

ForNode::ForNode(const uint32_t line, const uint32_t col, DeclNode *p_var_decl,
                 AssignmentNode *p_initial_statement,
                 ConstantValueNode *p_condition, CompoundStatementNode *p_body)
    : AstNode{line, col}, var_decl(p_var_decl),
      initial_statement(p_initial_statement), condition(p_condition),
      body(p_body), symbol_table(nullptr) {}

const ConstantValueNode *ForNode::getLowerBoundNode() const {
    auto *initial_value_node =
        dynamic_cast<const ConstantValueNode *>(initial_statement->getExpr());

    assert(initial_value_node && "Shouldn't reach here since the syntax has "
                                 "ensured that it will be a constant value");

    return initial_value_node;
}

const ConstantValueNode *ForNode::getUpperBoundNode() const {
    return condition.get();
}

void ForNode::setSymbolTable(const SymbolTable *table) { symbol_table = table; }

const SymbolTable *ForNode::getSymbolTable() const { return symbol_table; }

void ForNode::accept(AstNodeVisitor &p_visitor) { p_visitor.visit(*this); }

void ForNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    var_decl->accept(p_visitor);
    initial_statement->accept(p_visitor);
    condition->accept(p_visitor);
    body->accept(p_visitor);
}
