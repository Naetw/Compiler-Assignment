#include "codegen/CodeGenerator.hpp"
#include "AST/operator.hpp"
#include "visitor/AstNodeInclude.hpp"

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>

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
    "    .align 2\n"
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
    "    .size %s, .-%s";
// clang-format on

void CodeGenerator::visit(ProgramNode &p_program) {
    // clang-format off
    constexpr const char*const riscv_assembly_file_prologue =
        "    .file \"%s\"\n"
        "    .option nopic\n"
        ".section    .text\n";
    // clang-format on
    emitInstructions(m_output_file.get(), riscv_assembly_file_prologue,
                     m_source_file_path.c_str());

    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_program.getSymbolTable());
    m_context_stack.push(CodegenContext::kGlobal);

    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(*this); };
    for_each(p_program.getDeclNodes().begin(), p_program.getDeclNodes().end(),
             visit_ast_node);
    emitInstructions(m_output_file.get(), ".section    .text\n");
    for_each(p_program.getFuncNodes().begin(), p_program.getFuncNodes().end(),
             visit_ast_node);

    emitInstructions(m_output_file.get(), kFixedFunctionPrologue, "main",
                     "main", "main");
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

void CodeGenerator::visit(FunctionNode &p_function) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_function.getSymbolTable());

    p_function.visitChildNodes(*this);

    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_function.getSymbolTable());
}

void CodeGenerator::visit(CompoundStatementNode &p_compound_statement) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_compound_statement.getSymbolTable());
    m_context_stack.push(CodegenContext::kLocal);

    // start from 8 since 0-4, 4-8 are for return addr, last stack addr
    m_local_var_offset = 8;
    // TODO: need to store previous offset for multiple compound statement

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

void CodeGenerator::visit(FunctionInvocationNode &p_func_invocation) {}

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

void CodeGenerator::visit(ReadNode &p_read) {}

void CodeGenerator::visit(IfNode &p_if) {}

void CodeGenerator::visit(WhileNode &p_while) {}

void CodeGenerator::visit(ForNode &p_for) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_for.getSymbolTable());

    p_for.visitChildNodes(*this);

    m_symbol_manager_ptr->removeSymbolsFromHashTable(p_for.getSymbolTable());
}

void CodeGenerator::visit(ReturnNode &p_return) {}
