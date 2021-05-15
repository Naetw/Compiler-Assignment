#ifndef AST_PROGRAM_NODE_H
#define AST_PROGRAM_NODE_H

#include "AST/ast.hpp"
#include "AST/decl.hpp"

#include <memory>
#include <string>
#include <vector>

class ProgramNode final : public AstNode {
  public:
    using DeclNodes = std::vector<std::unique_ptr<DeclNode>>;

  private:
    std::string m_name;
    std::unique_ptr<PType> m_ret_type;
    DeclNodes m_decl_nodes;
    std::unique_ptr<CompoundStatementNode> m_body;
    // TODO: functions

  public:
    ~ProgramNode() = default;
    ProgramNode(const uint32_t line, const uint32_t col,
                const char *const p_name, PType *const p_ret_type,
                DeclNodes &p_decl_nodes, CompoundStatementNode *const p_body
                /* TODO: functions */)
        : AstNode{line, col}, m_name(p_name), m_ret_type(p_ret_type),
          m_decl_nodes(std::move(p_decl_nodes)), m_body(p_body) {}

    const char *getNameCString() const { return m_name.c_str(); }

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }

    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
