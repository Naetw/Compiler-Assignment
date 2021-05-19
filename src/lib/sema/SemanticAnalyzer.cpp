#include "sema/SemanticAnalyzer.hpp"
#include "sema/error.hpp"
#include "visitor/AstNodeInclude.hpp"

#include <algorithm>

static constexpr const char *kRedeclaredSymbolErrorMessage =
    "symbol '%s' is redeclared";

void SemanticAnalyzer::visit(ProgramNode &p_program) {
    m_symbol_manager.pushGlobalScope();
    m_context_stack.push(SemanticContext::kGlobal);

    auto success = m_symbol_manager.addSymbol(
        p_program.getName(), SymbolEntry::KindEnum::kProgramKind,
        p_program.getTypePtr(), static_cast<Constant *>(nullptr));
    if (!success) {
        logSemanticError(p_program.getLocation(), kRedeclaredSymbolErrorMessage,
                         p_program.getNameCString());
        m_has_error = true;
    }

    p_program.visitChildNodes(*this);

    /*
     * TODO:
     *
     * 4. Perform semantic analyses of this node.
     */

    m_context_stack.pop();
    m_symbol_manager.popGlobalScope();
}

void SemanticAnalyzer::visit(DeclNode &p_decl) {
    p_decl.visitChildNodes(*this);
}

SymbolEntry::KindEnum
SemanticAnalyzer::determineVarKind(const VariableNode &p_variable) {
    if (isInForLoop()) {
        return SymbolEntry::KindEnum::kLoopVarKind;
    }

    if (isInFunction()) {
        return SymbolEntry::KindEnum::kParameterKind;
    }

    // global or local
    return p_variable.getConstantPtr() ? SymbolEntry::KindEnum::kConstantKind
                                       : SymbolEntry::KindEnum::kVariableKind;
}

SymbolEntry *SemanticAnalyzer::addSymbol(const VariableNode &p_variable) {
    auto kind = determineVarKind(p_variable);

    auto *entry = m_symbol_manager.addSymbol(p_variable.getName(), kind,
                                             p_variable.getTypePtr(),
                                             p_variable.getConstantPtr());
    if (!entry) {
        logSemanticError(p_variable.getLocation(),
                         kRedeclaredSymbolErrorMessage,
                         p_variable.getNameCString());
        m_has_error = true;
    }

    return entry;
}

static bool validateDimensions(const VariableNode &p_variable) {
    bool has_error = false;

    auto validate_dimension = [&](const auto dimension) {
        if (dimension == 0) {
            logSemanticError(p_variable.getLocation(),
                             "'%s' declared as an array with an index that is "
                             "not greater than 0",
                             p_variable.getNameCString());
            has_error = true;
        }
    };

    for_each(p_variable.getTypePtr()->getDimensions().begin(),
             p_variable.getTypePtr()->getDimensions().end(),
             validate_dimension);

    return !has_error;
}

void SemanticAnalyzer::visit(VariableNode &p_variable) {
    auto *entry = addSymbol(p_variable);

    p_variable.visitChildNodes(*this);

    if (entry && !validateDimensions(p_variable)) {
        m_error_entry_set.insert(entry);
        m_has_error = true;
    }
}

void SemanticAnalyzer::visit(ConstantValueNode &p_constant_value) {
    p_constant_value.setInferredType(
        p_constant_value.getTypePtr()->getStructElementType(0));
}

void SemanticAnalyzer::visit(FunctionNode &p_function) {
    auto success = m_symbol_manager.addSymbol(
        p_function.getName(), SymbolEntry::KindEnum::kFunctionKind,
        p_function.getTypePtr(), &p_function.getParameters());
    if (!success) {
        logSemanticError(p_function.getLocation(),
                         kRedeclaredSymbolErrorMessage,
                         p_function.getNameCString());
        m_has_error = true;
    }

    m_symbol_manager.pushScope();
    m_context_stack.push(SemanticContext::kFunction);

    auto visit_ast_node = [this](auto &ast_node) { ast_node->accept(*this); };
    for_each(p_function.getParameters().begin(),
             p_function.getParameters().end(), visit_ast_node);

    // directly visit the body to prevent pushing duplicate scope
    m_context_stack.push(SemanticContext::kLocal);
    p_function.visitBodyChildNodes(*this);
    m_context_stack.pop();

    /*
     * TODO:
     *
     * 4. Perform semantic analyses of this node.
     */

    m_context_stack.pop();
    m_symbol_manager.popScope();
}

void SemanticAnalyzer::visit(CompoundStatementNode &p_compound_statement) {
    m_symbol_manager.pushScope();
    m_context_stack.push(SemanticContext::kLocal);

    p_compound_statement.visitChildNodes(*this);

    m_context_stack.pop();
    m_symbol_manager.popScope();
}

void SemanticAnalyzer::visit(PrintNode &p_print) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(BinaryOperatorNode &p_bin_op) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(UnaryOperatorNode &p_un_op) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(FunctionInvocationNode &p_func_invocation) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

static const SymbolEntry *
checkVariableExistence(const SymbolManager &p_symbol_manager,
                       const VariableReferenceNode &p_variable_ref) {
    const auto *entry = p_symbol_manager.lookup(p_variable_ref.getName());

    if (entry == nullptr) {
        logSemanticError(p_variable_ref.getLocation(),
                         "use of undeclared symbol '%s'",
                         p_variable_ref.getNameCString());
    }

    return entry;
}

static bool validateVariableKind(const SymbolEntry::KindEnum kind,
                                 const VariableReferenceNode &p_variable_ref) {
    if (kind != SymbolEntry::KindEnum::kParameterKind &&
        kind != SymbolEntry::KindEnum::kVariableKind &&
        kind != SymbolEntry::KindEnum::kLoopVarKind &&
        kind != SymbolEntry::KindEnum::kConstantKind) {
        logSemanticError(p_variable_ref.getLocation(),
                         "use of non-variable symbol '%s'",
                         p_variable_ref.getNameCString());
        return false;
    }
    return true;
}

static bool
validateArrayReference(const VariableReferenceNode &p_variable_ref) {
    for (const auto &index : p_variable_ref.getIndices()) {
        if (index->getInferredType() == nullptr) {
            return false;
        }

        if (!index->getInferredType()->isInteger()) {
            logSemanticError(index->getLocation(),
                             "index of array reference must be an integer");
            return false;
        }
    }

    return true;
}

static bool
validateArraySubscriptNum(const PType *p_var_type,
                          const VariableReferenceNode &p_variable_ref) {
    if (p_variable_ref.getIndices().size() >
        p_var_type->getDimensions().size()) {
        logSemanticError(p_variable_ref.getLocation(),
                         "there is an over array subscript on '%s'",
                         p_variable_ref.getNameCString());
        return false;
    }
    return true;
}

void SemanticAnalyzer::visit(VariableReferenceNode &p_variable_ref) {
    p_variable_ref.visitChildNodes(*this);

    const SymbolEntry *entry = nullptr;
    if ((entry = checkVariableExistence(m_symbol_manager, p_variable_ref)) ==
        nullptr) {
        return;
    }

    if (!validateVariableKind(entry->getKind(), p_variable_ref)) {
        return;
    }

    if (m_error_entry_set.find(const_cast<SymbolEntry *>(entry)) !=
        m_error_entry_set.end()) {
        return;
    }

    if (!validateArrayReference(p_variable_ref)) {
        return;
    }

    if (!validateArraySubscriptNum(entry->getTypePtr(), p_variable_ref)) {
        return;
    }

    p_variable_ref.setInferredType(entry->getTypePtr()->getStructElementType(
        p_variable_ref.getIndices().size()));
}

void SemanticAnalyzer::visit(AssignmentNode &p_assignment) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(ReadNode &p_read) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(IfNode &p_if) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(WhileNode &p_while) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}

void SemanticAnalyzer::visit(ForNode &p_for) {
    m_symbol_manager.pushScope();
    m_context_stack.push(SemanticContext::kForLoop);

    p_for.visitChildNodes(*this);

    /*
     * TODO:
     *
     * 4. Perform semantic analyses of this node.
     */

    m_context_stack.pop();
    m_symbol_manager.popScope();
}

void SemanticAnalyzer::visit(ReturnNode &p_return) {
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
}
