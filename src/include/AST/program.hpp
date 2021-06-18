#ifndef AST_PROGRAM_NODE_H
#define AST_PROGRAM_NODE_H

#include "AST/ast.hpp"
#include "AST/decl.hpp"
#include "AST/function.hpp"

#include <memory>
#include <string>
#include <vector>

class SymbolTable;

class ProgramNode final : public AstNode {
  public:
    using DeclNodes = std::vector<std::unique_ptr<DeclNode>>;
    using FuncNodes = std::vector<std::unique_ptr<FunctionNode>>;

  private:
    std::string m_name;
    std::unique_ptr<PType> m_ret_type;
    DeclNodes m_decl_nodes;
    FuncNodes m_func_nodes;
    std::unique_ptr<CompoundStatementNode> m_body;

    const SymbolTable *m_symbol_table_ptr = nullptr;

  public:
    ~ProgramNode() = default;
    ProgramNode(const uint32_t line, const uint32_t col,
                const char *const p_name, PType *const p_ret_type,
                DeclNodes &p_decl_nodes, FuncNodes &p_func_nodes,
                CompoundStatementNode *const p_body)
        : AstNode{line, col}, m_name(p_name), m_ret_type(p_ret_type),
          m_decl_nodes(std::move(p_decl_nodes)),
          m_func_nodes(std::move(p_func_nodes)), m_body(p_body) {}

    const char *getNameCString() const { return m_name.c_str(); }
    const std::string &getName() const { return m_name; }

    const PType *getTypePtr() const { return m_ret_type.get(); }

    const DeclNodes &getDeclNodes() const { return m_decl_nodes; }
    const FuncNodes &getFuncNodes() const { return m_func_nodes; }
    const CompoundStatementNode &getBody() const { return *m_body.get(); }

    const SymbolTable *getSymbolTable() const { return m_symbol_table_ptr; }
    void setSymbolTable(const SymbolTable *p_symbol_table) {
        m_symbol_table_ptr = p_symbol_table;
    }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }

    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
