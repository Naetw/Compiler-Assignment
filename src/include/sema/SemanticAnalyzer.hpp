#ifndef SEMA_SEMANTIC_ANALYZER_H
#define SEMA_SEMANTIC_ANALYZER_H

#include "sema/SymbolTable.hpp"
#include "visitor/AstNodeVisitor.hpp"

#include <set>
#include <stack>

class SemanticAnalyzer final : public AstNodeVisitor {
  private:
    enum class SemanticContext : uint8_t {
        kGlobal,
        kFunction,
        kForLoop,
        kLocal
    };

  private:
    SymbolManager m_symbol_manager;
    std::stack<SemanticContext> m_context_stack;
    std::stack<const PType *> m_returned_type_stack;

    std::set<SymbolEntry *> m_error_entry_set;

    bool m_has_error = false;

  public:
    ~SemanticAnalyzer() = default;
    SemanticAnalyzer(const bool opt_dmp) : m_symbol_manager(opt_dmp) {}

    void visit(ProgramNode &p_program) override;
    void visit(DeclNode &p_decl) override;
    void visit(VariableNode &p_variable) override;
    void visit(ConstantValueNode &p_constant_value) override;
    void visit(FunctionNode &p_function) override;
    void visit(CompoundStatementNode &p_compound_statement) override;
    void visit(PrintNode &p_print) override;
    void visit(BinaryOperatorNode &p_bin_op) override;
    void visit(UnaryOperatorNode &p_un_op) override;
    void visit(FunctionInvocationNode &p_func_invocation) override;
    void visit(VariableReferenceNode &p_variable_ref) override;
    void visit(AssignmentNode &p_assignment) override;
    void visit(ReadNode &p_read) override;
    void visit(IfNode &p_if) override;
    void visit(WhileNode &p_while) override;
    void visit(ForNode &p_for) override;
    void visit(ReturnNode &p_return) override;

    bool hasError() const { return m_has_error; }

    const SymbolManager *getSymbolManager() const { return &m_symbol_manager; }

  private:
    bool isInForLoop() const {
        return m_context_stack.top() == SemanticContext::kForLoop;
    }
    bool isInFunction() const {
        return m_context_stack.top() == SemanticContext::kFunction;
    }
    SymbolEntry::KindEnum determineVarKind(const VariableNode &p_var_node);
    SymbolEntry *addSymbol(const VariableNode &p_var_node);
};

#endif
