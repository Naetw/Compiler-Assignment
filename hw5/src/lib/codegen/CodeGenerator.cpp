#include "codegen/CodeGenerator.hpp"
#include "AST/operator.hpp"
#include "visitor/AstNodeInclude.hpp"

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>

// -4: return address, -8: frame pointer of the last stack
constexpr const size_t kLocalVariableStartOffset = 12;

CodeGenerator::CodeGenerator(const std::string source_file_name,
                             const std::string save_path,
                             const SymbolManager *const p_symbol_manager)
    : m_symbol_manager_ptr(p_symbol_manager),
      m_source_file_path(source_file_name) {
    // FIXME: assume that the source file is always xxxx.p
    const std::string &real_path =
        (save_path == "") ? std::string{"."} : save_path;
    auto slash_pos = source_file_name.rfind("/");
    auto dot_pos = source_file_name.rfind(".");

    if (slash_pos != std::string::npos) {
        ++slash_pos;
    } else {
        slash_pos = 0;
    }
    std::string output_file_path(
        real_path + "/" +
        source_file_name.substr(slash_pos, dot_pos - slash_pos) + ".S");
    m_output_file.reset(fopen(output_file_path.c_str(), "w"));
    assert(m_output_file.get() && "Failed to open output file");
}

static void emitInstructions(FILE *p_out_file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(p_out_file, format, args);
    va_end(args);
}

// clang-format off
static constexpr const char*const kFixedFunctionPrologue =
    "    .globl %s\n"
    "    .type %s, @function\n"
    "%s:\n"
    "    addi sp, sp, -128\n"
    "    sw ra, 124(sp)\n"
    "    sw s0, 120(sp)\n"
    "    addi s0, sp, 128\n";

static constexpr const char*const kFixedFunctionEpilogue =
    "    lw ra, 124(sp)\n"
    "    lw s0, 120(sp)\n"
    "    addi sp, sp, 128\n"
    "    jr ra\n"
    "    .size %s, .-%s\n";
// clang-format on

void CodeGenerator::visit(ProgramNode &p_program) {
    // clang-format off
    constexpr const char*const riscv_assembly_file_prologue =
        "    .file \"%s\"\n"
        "    .option nopic\n"
        ".section    .text\n"
        "    .align 2\n";
    // clang-format on
    emitInstructions(m_output_file.get(), riscv_assembly_file_prologue,
                     m_source_file_path.c_str());

    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_program.getSymbolTable());
    m_context_stack.push(CodegenContext::kGlobal);

    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(*this); };
    for_each(p_program.getDeclNodes().begin(), p_program.getDeclNodes().end(),
             visit_ast_node);
    emitInstructions(m_output_file.get(), ".section    .text\n"
                                          "    .align 2\n");
    for_each(p_program.getFuncNodes().begin(), p_program.getFuncNodes().end(),
             visit_ast_node);

    emitInstructions(m_output_file.get(), kFixedFunctionPrologue, "main",
                     "main", "main");
    // start from 8 since 0-4, 4-8 are for return addr, last stack addr
    m_local_var_offset = kLocalVariableStartOffset;
    const_cast<CompoundStatementNode &>(p_program.getBody()).accept(*this);
    emitInstructions(m_output_file.get(), kFixedFunctionEpilogue, "main",
                     "main");

    m_context_stack.pop();
    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_program.getSymbolTable());
}

void CodeGenerator::visit(DeclNode &p_decl) { p_decl.visitChildNodes(*this); }

void CodeGenerator::visit(VariableNode &p_variable) {
    assert(p_variable.getTypePtr()->isInteger() &&
           "cannot handle non-integer variable");

    const auto *constant_ptr = p_variable.getConstantPtr();
    if (isInGlobal(m_context_stack)) {
        if (constant_ptr) {
            emitInstructions(
                m_output_file.get(),
                ".section    .rodata\n"
                "    .align 2\n"
                "    .globl %s\n"
                "    .type %s, @object\n"
                "%s:\n"
                "    .word %d\n",
                p_variable.getNameCString(), p_variable.getNameCString(),
                p_variable.getNameCString(), constant_ptr->integer());
        } else {
            emitInstructions(m_output_file.get(), ".comm %s, 4, 4\n",
                             p_variable.getNameCString());
        }
        return;
    }

    if (isInLocal(m_context_stack)) {
        m_local_var_offset_map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(
                m_symbol_manager_ptr->lookup(p_variable.getName())),
            std::forward_as_tuple(m_local_var_offset));

        if (constant_ptr) {
            emitInstructions(m_output_file.get(),
                             "    li t0, %d\n"
                             "    sw t0, -%u(s0)\n",
                             constant_ptr->integer(), m_local_var_offset);
        }

        m_local_var_offset += 4;

        return;
    }

    assert(false &&
           "Shouln't reach here. It means that the context has wrong value");
}

void CodeGenerator::visit(ConstantValueNode &p_constant_value) {
    emitInstructions(m_output_file.get(),
                     "    li t0, %d\n"
                     "    addi sp, sp, -4\n"
                     "    sw t0, 0(sp)\n",
                     p_constant_value.getConstantPtr()->integer());
}

void CodeGenerator::storeArgumentsToParameters(
    const FunctionNode::DeclNodes &p_parameters) {
    constexpr size_t kNumOfArgumentRegister = 8;
    size_t index = 0;

    for (const auto &parameter : p_parameters) {
        for (const auto &var_node_ptr : parameter->getVariables()) {
            const auto *entry_ptr =
                m_symbol_manager_ptr->lookup(var_node_ptr->getName());
            auto search = m_local_var_offset_map.find(entry_ptr);
            assert(search != m_local_var_offset_map.end() &&
                   "Should have been defined before use");

            if (index < kNumOfArgumentRegister) {
                emitInstructions(m_output_file.get(), "    sw a%u, -%u(s0)\n",
                                 index, search->second);
            } else {
                emitInstructions(m_output_file.get(),
                                 "    lw t0, %u(s0)\n"
                                 "    sw t0, -%u(s0)\n",
                                 4 * (index - kNumOfArgumentRegister),
                                 search->second);
            }
            ++index;
        }
    }
}

void CodeGenerator::visit(FunctionNode &p_function) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_function.getSymbolTable());
    m_context_stack.push(CodegenContext::kLocal);

    emitInstructions(m_output_file.get(), kFixedFunctionPrologue,
                     p_function.getNameCString(), p_function.getNameCString(),
                     p_function.getNameCString());

    // start from 8 since 0-4, 4-8 are for return addr, last stack addr
    m_local_var_offset = kLocalVariableStartOffset;

    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(*this); };
    for_each(p_function.getParameters().begin(),
             p_function.getParameters().end(), visit_ast_node);

    storeArgumentsToParameters(p_function.getParameters());

    p_function.visitBodyChildNodes(*this);

    emitInstructions(m_output_file.get(), kFixedFunctionEpilogue,
                     p_function.getNameCString(), p_function.getNameCString());

    m_context_stack.pop();
    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_function.getSymbolTable());
}

void CodeGenerator::visit(CompoundStatementNode &p_compound_statement) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_compound_statement.getSymbolTable());
    m_context_stack.push(CodegenContext::kLocal);

    p_compound_statement.visitChildNodes(*this);

    m_context_stack.pop();
    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_compound_statement.getSymbolTable());
}

void CodeGenerator::visit(PrintNode &p_print) {
    m_ref_to_value = true;
    p_print.visitChildNodes(*this);

    emitInstructions(m_output_file.get(), "    lw a0, 0(sp)\n"
                                          "    addi sp, sp, 4\n"
                                          "    jal ra, printInt\n");
}

void CodeGenerator::visit(BinaryOperatorNode &p_bin_op) {
    p_bin_op.visitChildNodes(*this);

    emitInstructions(m_output_file.get(), "    lw t0, 0(sp)\n"
                                          "    addi sp, sp, 4\n"
                                          "    lw t1, 0(sp)\n"
                                          "    addi sp, sp, 4\n");

    switch (p_bin_op.getOp()) {
    case Operator::kMultiplyOp:
        emitInstructions(m_output_file.get(), "    mul t0, t1, t0\n");
        break;
    case Operator::kDivideOp:
        emitInstructions(m_output_file.get(), "    div t0, t1, t0\n");
        break;
    case Operator::kModOp:
        emitInstructions(m_output_file.get(), "    rem t0, t1, t0\n");
        break;
    case Operator::kPlusOp:
        emitInstructions(m_output_file.get(), "    add t0, t1, t0\n");
        break;
    case Operator::kMinusOp:
        emitInstructions(m_output_file.get(), "    sub t0, t1, t0\n");
        break;
    case Operator::kLessOp:
        emitInstructions(m_output_file.get(),
                         "    blt t1, t0, L%u\n"
                         "    j L%u\n",
                         m_comp_branch_true_label, m_comp_branch_false_label);
        return;
    case Operator::kLessOrEqualOp:
        emitInstructions(m_output_file.get(),
                         "    ble t1, t0, L%u\n"
                         "    j L%u\n",
                         m_comp_branch_true_label, m_comp_branch_false_label);
        return;
    case Operator::kGreaterOp:
        emitInstructions(m_output_file.get(),
                         "    bgt t1, t0, L%u\n"
                         "    j L%u\n",
                         m_comp_branch_true_label, m_comp_branch_false_label);
        return;
    case Operator::kGreaterOrEqualOp:
        emitInstructions(m_output_file.get(),
                         "    bge t1, t0, L%u\n"
                         "    j L%u\n",
                         m_comp_branch_true_label, m_comp_branch_false_label);
        return;
    case Operator::kEqualOp:
        emitInstructions(m_output_file.get(),
                         "    beq t1, t0, L%u\n"
                         "    j L%u\n",
                         m_comp_branch_true_label, m_comp_branch_false_label);
        return;
    case Operator::kNotEqualOp:
        emitInstructions(m_output_file.get(),
                         "    bne t1, t0, L%u\n"
                         "    j L%u\n",
                         m_comp_branch_true_label, m_comp_branch_false_label);
        return;
    default:
        assert(false && "unsupported binary operator");
        return;
    }
    emitInstructions(m_output_file.get(), "    addi sp, sp, -4\n"
                                          "    sw t0, 0(sp)\n");
}

void CodeGenerator::visit(UnaryOperatorNode &p_un_op) {
    p_un_op.visitChildNodes(*this);

    emitInstructions(m_output_file.get(), "    lw t0, 0(sp)\n"
                                          "    addi sp, sp, 4\n");

    switch (p_un_op.getOp()) {
    case Operator::kNegOp:
        emitInstructions(m_output_file.get(), "    sub t0, zero, t0\n");
        break;
    default:
        assert(false && "unsupported unary operator");
        return;
    }
    emitInstructions(m_output_file.get(), "    addi sp, sp, -4\n"
                                          "    sw t0, 0(sp)\n");
}

void CodeGenerator::visit(FunctionInvocationNode &p_func_invocation) {
    constexpr size_t kNumOfArgumentRegister = 8;
    const auto &arguments = p_func_invocation.getArguments();

    m_ref_to_value = true;
    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(*this); };
    if (arguments.size() > kNumOfArgumentRegister) {
        // [0, kNumOfArgumentRegister)
        for_each(arguments.begin(),
                 std::next(arguments.begin(), kNumOfArgumentRegister),
                 visit_ast_node);
    } else {
        for_each(arguments.begin(), arguments.end(), visit_ast_node);
    }

    // RISC-V has a0-a7 for passing arguments
    size_t num_of_a_reg = std::min(kNumOfArgumentRegister, arguments.size());
    for (size_t i = 0; i < num_of_a_reg; ++i) {
        emitInstructions(m_output_file.get(),
                         "    lw a%u, 0(sp)\n"
                         "    addi sp, sp, 4\n",
                         num_of_a_reg - i - 1);
    }

    // [kNumOfArgumentRegister, end)
    if (arguments.size() > kNumOfArgumentRegister) {
        for_each(std::next(arguments.begin(), kNumOfArgumentRegister),
                 arguments.end(), visit_ast_node);
    }

    emitInstructions(m_output_file.get(), "    jal ra, %s\n",
                     p_func_invocation.getNameCString());

    // restore the stack if necessary
    if (arguments.size() > kNumOfArgumentRegister) {
        emitInstructions(m_output_file.get(), "    addi sp, sp, %u\n",
                         4 * (arguments.size() - kNumOfArgumentRegister));
    }

    emitInstructions(m_output_file.get(), "    mv t0, a0\n"
                                          "    addi sp, sp, -4\n"
                                          "    sw t0, 0(sp)\n");
}

void CodeGenerator::visit(VariableReferenceNode &p_variable_ref) {
    // Haven't supported array reference
    const auto *entry_ptr =
        m_symbol_manager_ptr->lookup(p_variable_ref.getName());
    auto search = m_local_var_offset_map.find(entry_ptr);
    if (search == m_local_var_offset_map.end()) {
        // global variable reference
        emitInstructions(m_output_file.get(), "    la t0, %s\n",
                         p_variable_ref.getNameCString());
    } else {
        // local variable reference
        emitInstructions(m_output_file.get(), "    addi t0, s0, -%u\n",
                         search->second);
    }

    // dereference to get the value if needed
    if (m_ref_to_value) {
        emitInstructions(m_output_file.get(), "    lw t1, 0(t0)\n"
                                              "    mv t0, t1\n");
    }

    // push onto stack
    emitInstructions(m_output_file.get(), "    addi sp, sp, -4\n"
                                          "    sw t0, 0(sp)\n");
}

void CodeGenerator::visit(AssignmentNode &p_assignment) {
    m_ref_to_value = false;
    const_cast<VariableReferenceNode &>(p_assignment.getLvalue()).accept(*this);
    m_ref_to_value = true;
    const_cast<ExpressionNode &>(p_assignment.getExpr()).accept(*this);

    emitInstructions(m_output_file.get(), "    lw t0, 0(sp)\n"
                                          "    addi sp, sp, 4\n"
                                          "    lw t1, 0(sp)\n"
                                          "    addi sp, sp, 4\n"
                                          "    sw t0, 0(t1)\n");
}

void CodeGenerator::visit(ReadNode &p_read) {
    m_ref_to_value = false; 
    p_read.visitChildNodes(*this);
    
    emitInstructions(
        m_output_file.get(),
        "    jal ra, readInt\n"
        "    lw t0, 0(sp)\n"
        "    addi sp, sp, 4\n"
        "    sw a0, 0(t0)\n");
}

void CodeGenerator::visit(IfNode &p_if) {
    const auto if_body_label = m_label_sequence;
    ++m_label_sequence;

    const auto *else_body_ptr = p_if.getElseBodyPtr();
    const size_t else_body_label = (else_body_ptr) ? m_label_sequence++ : 0;

    const auto out_label = m_label_sequence;
    ++m_label_sequence;

    m_comp_branch_true_label = if_body_label;
    m_comp_branch_false_label = (else_body_ptr) ? else_body_label : out_label;
    m_ref_to_value = true;
    const_cast<ExpressionNode &>(p_if.getCondition()).accept(*this);

    emitInstructions(m_output_file.get(), "L%u:\n", if_body_label);
    const_cast<CompoundStatementNode &>(p_if.getIfBody()).accept(*this);

    if (else_body_ptr) {
        // TODO: cannot handle nested compound statements
        emitInstructions(m_output_file.get(),
                         "    j L%u\n"
                         "L%u:\n",
                         out_label, else_body_label);
        const_cast<CompoundStatementNode *>(else_body_ptr)->accept(*this);
    }
    emitInstructions(m_output_file.get(), "L%u:\n", out_label);
}

void CodeGenerator::visit(WhileNode &p_while) {
    const auto while_head_label = m_label_sequence;
    const auto while_body_label = m_label_sequence + 1;
    const auto while_out_label = m_label_sequence + 2;
    m_label_sequence += 3;

    emitInstructions(m_output_file.get(), "L%u:\n", while_head_label);
    m_comp_branch_true_label = while_body_label;
    m_comp_branch_false_label = while_out_label;
    m_ref_to_value = true;
    const_cast<ExpressionNode &>(p_while.getCondition()).accept(*this);

    emitInstructions(m_output_file.get(), "L%u:\n", while_body_label);
    const_cast<CompoundStatementNode &>(p_while.getBody()).accept(*this);
    // TODO: cannot handle nested compound statements
    emitInstructions(m_output_file.get(),
                     "    j L%u\n"
                     "L%u:\n",
                     while_head_label, while_out_label);
}

void CodeGenerator::visit(ForNode &p_for) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_for.getSymbolTable());
    m_context_stack.push(CodegenContext::kLocal);

    const_cast<DeclNode &>(p_for.getLoopVarDecl()).accept(*this);
    const_cast<AssignmentNode &>(p_for.getLoopVarInitStmt()).accept(*this);

    const auto for_head_label = m_label_sequence;
    const auto for_body_label = m_label_sequence + 1;
    const auto for_out_label = m_label_sequence + 2;
    m_label_sequence += 3;

    emitInstructions(m_output_file.get(), "L%u:\n", for_head_label);

    // hand-written comparison
    const auto *entry_ptr =
        m_symbol_manager_ptr->lookup(p_for.getLoopVarName());
    auto search = m_local_var_offset_map.find(entry_ptr);
    assert(search != m_local_var_offset_map.end() &&
           "Should have been defined before use");
    emitInstructions(m_output_file.get(),
                     "    lw t1, -%u(s0)\n"
                     "    li t0, %u\n"
                     "    blt t1, t0, L%u\n"
                     "    j L%u\n",
                     search->second,
                     p_for.getUpperBound().getConstantPtr()->integer(),
                     for_body_label, for_out_label);

    emitInstructions(m_output_file.get(), "L%u:\n", for_body_label);
    const_cast<CompoundStatementNode &>(p_for.getBody()).accept(*this);

    // loop_var += 1 & jump back to head for condition check
    emitInstructions(m_output_file.get(),
                     "    lw t0, -%u(s0)\n"
                     "    li t1, 1\n"
                     "    add t0, t0, t1\n"
                     "    sw t0, -%u(s0)\n",
                     search->second, search->second);
    // TODO: cannot handle nested compound statements
    emitInstructions(m_output_file.get(),
                     "    j L%u\n"
                     "L%u:\n",
                     for_head_label, for_out_label);

    m_context_stack.pop();
    m_symbol_manager_ptr->removeSymbolsFromHashTable(p_for.getSymbolTable());
}

void CodeGenerator::visit(ReturnNode &p_return) {
    m_ref_to_value = true;
    p_return.visitChildNodes(*this);

    emitInstructions(m_output_file.get(), "    lw t0, 0(sp)\n"
                                          "    addi sp, sp, 4\n"
                                          "    mv a0, t0\n");
}
