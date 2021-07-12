#include "sema/SemanticAnalyzer.hpp"
#include "sema/error.hpp"
#include "visitor/AstNodeInclude.hpp"

#include <algorithm>
#include <cassert>

static constexpr const char *kRedeclaredSymbolErrorMessage =
    "symbol '%s' is redeclared";

void SemanticAnalyzer::visit(ProgramNode &p_program) {
    m_symbol_manager.pushGlobalScope();
    m_context_stack.push(SemanticContext::kGlobal);
    m_returned_type_stack.push(p_program.getTypePtr());

    auto success = m_symbol_manager.addSymbol(
        p_program.getName(), SymbolEntry::KindEnum::kProgramKind,
        p_program.getTypePtr(), static_cast<Constant *>(nullptr));
    if (!success) {
        logSemanticError(p_program.getLocation(), kRedeclaredSymbolErrorMessage,
                         p_program.getNameCString());
        m_has_error = true;
    }

    p_program.visitChildNodes(*this);

    p_program.setSymbolTable(m_symbol_manager.getCurrentTable());

    m_returned_type_stack.pop();
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
    m_returned_type_stack.push(p_function.getTypePtr());

    auto visit_ast_node = [this](auto &ast_node) { ast_node->accept(*this); };
    for_each(p_function.getParameters().begin(),
             p_function.getParameters().end(), visit_ast_node);

    // directly visit the body to prevent pushing duplicate scope
    m_context_stack.push(SemanticContext::kLocal);
    p_function.visitBodyChildNodes(*this);
    m_context_stack.pop();

    p_function.setSymbolTable(m_symbol_manager.getCurrentTable());

    m_returned_type_stack.pop();
    m_context_stack.pop();
    m_symbol_manager.popScope();
}

void SemanticAnalyzer::visit(CompoundStatementNode &p_compound_statement) {
    m_symbol_manager.pushScope();
    m_context_stack.push(SemanticContext::kLocal);

    p_compound_statement.visitChildNodes(*this);

    p_compound_statement.setSymbolTable(m_symbol_manager.getCurrentTable());

    m_context_stack.pop();
    m_symbol_manager.popScope();
}

static bool validatePrintTarget(const PrintNode &p_print) {
    const auto *const target_type_ptr = p_print.getTarget().getInferredType();
    if (!target_type_ptr) {
        return false;
    }

    if (!target_type_ptr->isScalar()) {
        logSemanticError(p_print.getTarget().getLocation(),
                         "expression of print statement must be scalar type");
        return false;
    }

    return true;
}

void SemanticAnalyzer::visit(PrintNode &p_print) {
    p_print.visitChildNodes(*this);

    if (!validatePrintTarget(p_print)) {
        m_has_error = true;
    }
}

static bool validateOperandsInArithmeticOp(const Operator op,
                                           const PType *const p_left_type,
                                           const PType *const p_right_type) {
    if (op == Operator::kPlusOp && p_left_type->isString() &&
        p_right_type->isString()) {
        return true;
    }

    if ((p_left_type->isInteger() || p_left_type->isReal()) &&
        (p_right_type->isInteger() || p_right_type->isReal())) {
        return true;
    }

    return false;
}

static bool validateOperandsInModOp(const PType *const p_left_type,
                                    const PType *const p_right_type) {
    return p_left_type->isInteger() && p_right_type->isInteger();
}

static bool validateOperandsInBooleanOp(const PType *const p_left_type,
                                        const PType *const p_right_type) {
    return p_left_type->isBool() && p_right_type->isBool();
}

static bool validateOperandsInRelationalOp(const PType *const p_left_type,
                                           const PType *const p_right_type) {
    return (p_left_type->isInteger() || p_left_type->isReal()) &&
           (p_right_type->isInteger() || p_right_type->isReal());
}

static bool validateBinaryOperands(BinaryOperatorNode &p_bin_op) {
    const auto *left_type_ptr = p_bin_op.getLeftOperand().getInferredType();
    const auto *right_type_ptr = p_bin_op.getRightOperand().getInferredType();

    if (left_type_ptr == nullptr || right_type_ptr == nullptr) {
        return false;
    }

    switch (p_bin_op.getOp()) {
    case Operator::kPlusOp:
    case Operator::kMinusOp:
    case Operator::kMultiplyOp:
    case Operator::kDivideOp:
        if (validateOperandsInArithmeticOp(p_bin_op.getOp(), left_type_ptr,
                                           right_type_ptr)) {
            return true;
        }
        break;
    case Operator::kModOp:
        if (validateOperandsInModOp(left_type_ptr, right_type_ptr)) {
            return true;
        }
        break;
    case Operator::kAndOp:
    case Operator::kOrOp:
        if (validateOperandsInBooleanOp(left_type_ptr, right_type_ptr)) {
            return true;
        }
        break;
    case Operator::kLessOp:
    case Operator::kLessOrEqualOp:
    case Operator::kEqualOp:
    case Operator::kGreaterOp:
    case Operator::kGreaterOrEqualOp:
    case Operator::kNotEqualOp:
        if (validateOperandsInRelationalOp(left_type_ptr, right_type_ptr)) {
            return true;
        }
        break;
    default:
        assert(false && "unknown binary op or unary op");
    }

    logSemanticError(p_bin_op.getLocation(),
                     "invalid operands to binary operator '%s' ('%s' and '%s')",
                     p_bin_op.getOpCString(), left_type_ptr->getPTypeCString(),
                     right_type_ptr->getPTypeCString());
    return false;
}

static void setBinaryOpInferredType(BinaryOperatorNode &p_bin_op) {
    switch (p_bin_op.getOp()) {
    case Operator::kPlusOp:
    case Operator::kMinusOp:
    case Operator::kMultiplyOp:
    case Operator::kDivideOp:
        if (p_bin_op.getLeftOperand().getInferredType()->isString()) {
            p_bin_op.setInferredType(
                new PType(PType::PrimitiveTypeEnum::kStringType));
            return;
        }

        if (p_bin_op.getLeftOperand().getInferredType()->isReal() ||
            p_bin_op.getRightOperand().getInferredType()->isReal()) {
            p_bin_op.setInferredType(
                new PType(PType::PrimitiveTypeEnum::kRealType));
            return;
        }
    case Operator::kModOp:
        p_bin_op.setInferredType(
            new PType(PType::PrimitiveTypeEnum::kIntegerType));
        return;
    case Operator::kAndOp:
    case Operator::kOrOp:
        p_bin_op.setInferredType(
            new PType(PType::PrimitiveTypeEnum::kBoolType));
        return;
    case Operator::kLessOp:
    case Operator::kLessOrEqualOp:
    case Operator::kEqualOp:
    case Operator::kGreaterOp:
    case Operator::kGreaterOrEqualOp:
    case Operator::kNotEqualOp:
        p_bin_op.setInferredType(
            new PType(PType::PrimitiveTypeEnum::kBoolType));
        return;
    default:
        assert(false && "unknown binary op or unary op");
    }
}

void SemanticAnalyzer::visit(BinaryOperatorNode &p_bin_op) {
    p_bin_op.visitChildNodes(*this);

    if (!validateBinaryOperands(p_bin_op)) {
        m_has_error = true;
        return;
    }

    setBinaryOpInferredType(p_bin_op);
}

static bool validateUnaryOperand(const UnaryOperatorNode &p_un_op) {
    const auto *const operand_type = p_un_op.getOperand().getInferredType();
    if (!operand_type) {
        return false;
    }

    switch (p_un_op.getOp()) {
    case Operator::kNegOp:
        if (operand_type->isInteger() || operand_type->isReal()) {
            return true;
        }
        break;
    case Operator::kNotOp:
        if (operand_type->isBool()) {
            return true;
        }
        break;
    default:
        assert(false && "unknown binary op or unary op");
    }

    logSemanticError(p_un_op.getLocation(),
                     "invalid operand to unary operator '%s' ('%s')",
                     p_un_op.getOpCString(), operand_type->getPTypeCString());
    return false;
}

static void setUnaryOpInferredType(UnaryOperatorNode &p_un_op) {
    switch (p_un_op.getOp()) {
    case Operator::kNegOp:
        p_un_op.setInferredType(new PType(
            p_un_op.getOperand().getInferredType()->getPrimitiveType()));
        return;
    case Operator::kNotOp:
        p_un_op.setInferredType(new PType(PType::PrimitiveTypeEnum::kBoolType));
        return;
    default:
        assert(false && "unknown binary op or unary op");
    }
}

void SemanticAnalyzer::visit(UnaryOperatorNode &p_un_op) {
    p_un_op.visitChildNodes(*this);

    if (!validateUnaryOperand(p_un_op)) {
        m_has_error = true;
        return;
    }

    setUnaryOpInferredType(p_un_op);
}

static const SymbolEntry *
checkSymbolExistence(const SymbolManager &p_symbol_manager,
                     const std::string &p_name, const Location &p_location) {
    const auto *entry = p_symbol_manager.lookup(p_name);

    if (entry == nullptr) {
        logSemanticError(p_location, "use of undeclared symbol '%s'",
                         p_name.c_str());
    }

    return entry;
}

static bool validateFunctionInvocationKind(
    const SymbolEntry::KindEnum kind,
    const FunctionInvocationNode &p_func_invocation) {
    if (kind != SymbolEntry::KindEnum::kFunctionKind) {
        logSemanticError(p_func_invocation.getLocation(),
                         "call of non-function symbol '%s'",
                         p_func_invocation.getNameCString());
        return false;
    }
    return true;
}

static bool validateArguments(const SymbolEntry *const p_entry,
                              const FunctionInvocationNode &p_func_invocation) {
    const auto &parameters = *p_entry->getAttribute().parameters();
    const auto &arguments = p_func_invocation.getArguments();

    if (arguments.size() != FunctionNode::getParametersNum(parameters)) {
        logSemanticError(p_func_invocation.getLocation(),
                         "too few/much arguments provided for function '%s'",
                         p_func_invocation.getNameCString());
        return false;
    }

    FunctionInvocationNode::ExprNodes::const_iterator argument_iter =
        arguments.begin();

    for (const auto &parameter : parameters) {
        const auto &variables = parameter->getVariables();
        for (const auto &variable : variables) {
            auto *expr_type_ptr = (*argument_iter)->getInferredType();
            if (!expr_type_ptr) {
                return false;
            }

            if (!expr_type_ptr->compare(variable->getTypePtr())) {
                logSemanticError(
                    (*argument_iter)->getLocation(),
                    "incompatible type passing '%s' to parameter of type '%s'",
                    expr_type_ptr->getPTypeCString(),
                    variable->getTypePtr()->getPTypeCString());
                return false;
            }

            argument_iter++;
        }
    }

    return true;
}

static void
setFuncInvocationInferredType(FunctionInvocationNode &p_func_invocation,
                              const SymbolEntry *p_entry) {
    p_func_invocation.setInferredType(
        new PType(p_entry->getTypePtr()->getPrimitiveType()));
}

void SemanticAnalyzer::visit(FunctionInvocationNode &p_func_invocation) {
    p_func_invocation.visitChildNodes(*this);

    const SymbolEntry *entry = nullptr;
    if ((entry = checkSymbolExistence(
             m_symbol_manager, p_func_invocation.getName(),
             p_func_invocation.getLocation())) == nullptr) {
        m_has_error = true;
        return;
    }

    if (!validateFunctionInvocationKind(entry->getKind(), p_func_invocation)) {
        m_has_error = true;
        return;
    }

    if (!validateArguments(entry, p_func_invocation)) {
        m_has_error = true;
        return;
    }

    setFuncInvocationInferredType(p_func_invocation, entry);
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
    if ((entry =
             checkSymbolExistence(m_symbol_manager, p_variable_ref.getName(),
                                  p_variable_ref.getLocation())) == nullptr) {
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

static bool validateAssignmentLvalue(const AssignmentNode &p_assignment,
                                     const SymbolManager &p_symbol_manager,
                                     const bool is_in_for_loop) {
    const auto &lvalue = p_assignment.getLvalue();

    const auto *const lvalue_type_ptr = lvalue.getInferredType();
    if (!lvalue_type_ptr) {
        return false;
    }

    if (!lvalue_type_ptr->isScalar()) {
        logSemanticError(lvalue.getLocation(),
                         "array assignment is not allowed");
        return false;
    }

    const auto *const entry = p_symbol_manager.lookup(lvalue.getName());
    if (entry->getKind() == SymbolEntry::KindEnum::kConstantKind) {
        logSemanticError(lvalue.getLocation(),
                         "cannot assign to variable '%s' which is a constant",
                         lvalue.getNameCString());
        return false;
    }

    if (!is_in_for_loop &&
        entry->getKind() == SymbolEntry::KindEnum::kLoopVarKind) {
        logSemanticError(lvalue.getLocation(),
                         "the value of loop variable cannot be modified inside "
                         "the loop body");
        return false;
    }

    return true;
}

static bool validateAssignmentExpr(const AssignmentNode &p_assignment) {
    const auto &expr = p_assignment.getExpr();
    const auto *const expr_type_ptr = expr.getInferredType();
    if (!expr_type_ptr) {
        return false;
    }

    if (!expr_type_ptr->isScalar()) {
        logSemanticError(expr.getLocation(), "array assignment is not allowed");
        return false;
    }

    const auto *const lvalue_type_ptr =
        p_assignment.getLvalue().getInferredType();
    if (!lvalue_type_ptr->compare(expr_type_ptr)) {
        logSemanticError(p_assignment.getLocation(),
                         "assigning to '%s' from incompatible type '%s'",
                         lvalue_type_ptr->getPTypeCString(),
                         expr_type_ptr->getPTypeCString());
        return false;
    }

    return true;
}

void SemanticAnalyzer::visit(AssignmentNode &p_assignment) {
    p_assignment.visitChildNodes(*this);

    if (!validateAssignmentLvalue(p_assignment, m_symbol_manager,
                                  isInForLoop())) {
        m_has_error = true;
        return;
    }

    if (!validateAssignmentExpr(p_assignment)) {
        m_has_error = true;
        return;
    }
}

static bool validateReadTarget(const ReadNode &p_read,
                               const SymbolManager &p_symbol_manager) {
    const auto *const target_type_ptr = p_read.getTarget().getInferredType();
    if (!target_type_ptr) {
        return false;
    }

    if (!target_type_ptr->isScalar()) {
        logSemanticError(
            p_read.getTarget().getLocation(),
            "variable reference of read statement must be scalar type");
        return false;
    }

    const auto *const entry =
        p_symbol_manager.lookup(p_read.getTarget().getName());
    assert(entry && "Shouldn't reach here. This should be catched during the"
                    "visits of child nodes");

    if (entry->getKind() == SymbolEntry::KindEnum::kConstantKind ||
        entry->getKind() == SymbolEntry::KindEnum::kLoopVarKind) {
        logSemanticError(p_read.getTarget().getLocation(),
                         "variable reference of read statement cannot be a "
                         "constant or loop variable");
        return false;
    }

    return true;
}

void SemanticAnalyzer::visit(ReadNode &p_read) {
    p_read.visitChildNodes(*this);

    if (!validateReadTarget(p_read, m_symbol_manager)) {
        m_has_error = true;
    }
}

static bool validateConditionExpr(const ExpressionNode &p_condition) {
    const auto *const type_ptr = p_condition.getInferredType();
    if (!type_ptr) {
        return false;
    }

    if (!type_ptr->isBool()) {
        logSemanticError(p_condition.getLocation(),
                         "the expression of condition must be boolean type");
        return false;
    }

    return true;
}

void SemanticAnalyzer::visit(IfNode &p_if) {
    p_if.visitChildNodes(*this);

    if (!validateConditionExpr(p_if.getCondition())) {
        m_has_error = true;
    }
}

void SemanticAnalyzer::visit(WhileNode &p_while) {
    p_while.visitChildNodes(*this);

    if (!validateConditionExpr(p_while.getCondition())) {
        m_has_error = true;
    }
}

static bool validateForLoopBound(const ForNode &p_for) {
    auto initial_value = p_for.getLowerBound().getConstantPtr()->integer();
    auto condition_value = p_for.getUpperBound().getConstantPtr()->integer();

    if (initial_value >= condition_value) {
        logSemanticError(p_for.getLocation(),
                         "the lower bound and upper bound of iteration count "
                         "must be in the incremental order");
        return false;
    }

    return true;
}

void SemanticAnalyzer::visit(ForNode &p_for) {
    m_symbol_manager.pushScope();
    m_context_stack.push(SemanticContext::kForLoop);

    p_for.visitChildNodes(*this);

    if (!validateForLoopBound(p_for)) {
        m_has_error = true;
    }

    p_for.setSymbolTable(m_symbol_manager.getCurrentTable());

    m_context_stack.pop();
    m_symbol_manager.popScope();
}

static bool validateReturnValueType(const ExpressionNode &p_retval,
                                    const PType *const p_expected_return_type) {
    const auto *const retval_type_ptr = p_retval.getInferredType();
    if (!retval_type_ptr) {
        return false;
    }

    if (!p_expected_return_type->compare(retval_type_ptr)) {
        logSemanticError(p_retval.getLocation(),
                         "return '%s' from a function with return type '%s'",
                         retval_type_ptr->getPTypeCString(),
                         p_expected_return_type->getPTypeCString());
        return false;
    }

    return true;
}

void SemanticAnalyzer::visit(ReturnNode &p_return) {
    p_return.visitChildNodes(*this);

    const auto *const expected_return_type_ptr = m_returned_type_stack.top();
    if (expected_return_type_ptr->isVoid()) {
        logSemanticError(p_return.getLocation(),
                         "program/procedure should not return a value");
        m_has_error = true;
        return;
    }

    if (!validateReturnValueType(p_return.getReturnValue(),
                                 expected_return_type_ptr)) {
        m_has_error = true;
        return;
    }
}
