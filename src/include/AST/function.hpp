#ifndef AST_FUNCTION_NODE_H
#define AST_FUNCTION_NODE_H

#include "AST/CompoundStatement.hpp"
#include "AST/ast.hpp"
#include "visitor/AstNodeVisitor.hpp"

#include <memory>
#include <string>
#include <vector>

class FunctionNode final : public AstNode {
  public:
    using DeclNodes = std::vector<std::unique_ptr<DeclNode>>;

  private:
    std::string m_name;
    DeclNodes m_parameters;
    std::unique_ptr<PType> m_ret_type;
    std::unique_ptr<CompoundStatementNode> m_body;

    mutable std::string m_prototype_string;
    mutable bool m_prototype_string_is_valid = false;

  public:
    ~FunctionNode() = default;
    FunctionNode(const uint32_t line, const uint32_t col,
                 const char *const p_name, DeclNodes &p_decl_nodes,
                 PType *const p_ret_type, CompoundStatementNode *const p_body)
        : AstNode{line, col}, m_name(p_name),
          m_parameters(std::move(p_decl_nodes)), m_ret_type(p_ret_type),
          m_body(p_body) {}

    const char *getNameCString() const { return m_name.c_str(); }
    const char *getPrototypeCString() const;

    void accept(AstNodeVisitor &p_visitor) override { p_visitor.visit(*this); }
    void visitChildNodes(AstNodeVisitor &p_visitor) override;
};

#endif
