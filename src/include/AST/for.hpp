#ifndef AST_FOR_NODE_H
#define AST_FOR_NODE_H

#include "AST/ast.hpp"
#include "AST/decl.hpp"
#include "AST/assignment.hpp"
#include "AST/expression.hpp"
#include "AST/CompoundStatement.hpp"

class SymbolTable;

class ForNode final : public AstNode {
  private:
    std::unique_ptr<DeclNode> m_loop_var_decl;
    std::unique_ptr<AssignmentNode> m_init_stmt;
    std::unique_ptr<ExpressionNode> m_end_condition;
    std::unique_ptr<CompoundStatementNode> m_body;

    const SymbolTable *m_symbol_table_ptr = nullptr;

  public:
    ~ForNode() = default;
    ForNode(const uint32_t line, const uint32_t col,
            DeclNode *p_loop_var_decl, AssignmentNode *p_init_stmt,
            ExpressionNode *p_end_condition, CompoundStatementNode *p_body)
        : AstNode{line, col}, m_loop_var_decl(p_loop_var_decl),
          m_init_stmt(p_init_stmt), m_end_condition(p_end_condition),
          m_body(p_body) {}

    const ConstantValueNode &getLowerBound() const;
    const ConstantValueNode &getUpperBound() const;

    const std::string &getLoopVarName() const;

    const DeclNode &getLoopVarDecl() const { return *m_loop_var_decl.get(); }
    const AssignmentNode &getLoopVarInitStmt() const {
        return *m_init_stmt.get();
    }
    const CompoundStatementNode &getBody() const { return *m_body.get(); }


    const SymbolTable *getSymbolTable() const { return m_symbol_table_ptr; }
    void setSymbolTable(const SymbolTable *p_symbol_table) {
        m_symbol_table_ptr = p_symbol_table;
    }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }
    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
