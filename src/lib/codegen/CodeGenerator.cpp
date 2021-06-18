#include "codegen/CodeGenerator.hpp"
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

static void dumpInstructions(FILE *p_out_file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(p_out_file, format, args);
    va_end(args);
}

void CodeGenerator::visit(ProgramNode &p_program) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_program.getSymbolTable());

    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(*this); };
    for_each(p_program.getDeclNodes().begin(), p_program.getDeclNodes().end(),
             visit_ast_node);
    for_each(p_program.getFuncNodes().begin(), p_program.getFuncNodes().end(),
             visit_ast_node);

    const_cast<CompoundStatementNode &>(p_program.getBody()).accept(*this);

    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_program.getSymbolTable());
}

void CodeGenerator::visit(DeclNode &p_decl) {}

void CodeGenerator::visit(VariableNode &p_variable) {}

void CodeGenerator::visit(ConstantValueNode &p_constant_value) {}

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

    p_compound_statement.visitChildNodes(*this);

    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_compound_statement.getSymbolTable());
}

void CodeGenerator::visit(PrintNode &p_print) {}

void CodeGenerator::visit(BinaryOperatorNode &p_bin_op) {}

void CodeGenerator::visit(UnaryOperatorNode &p_un_op) {}

void CodeGenerator::visit(FunctionInvocationNode &p_func_invocation) {}

void CodeGenerator::visit(VariableReferenceNode &p_variable_ref) {}

void CodeGenerator::visit(AssignmentNode &p_assignment) {}

void CodeGenerator::visit(ReadNode &p_read) {}

void CodeGenerator::visit(IfNode &p_if) {}

void CodeGenerator::visit(WhileNode &p_while) {}

void CodeGenerator::visit(ForNode &p_for) {
    m_symbol_manager_ptr->reconstructHashTableFromSymbolTable(
        p_for.getSymbolTable());

    p_for.visitChildNodes(*this);

    m_symbol_manager_ptr->removeSymbolsFromHashTable(
        p_for.getSymbolTable());
}

void CodeGenerator::visit(ReturnNode &p_return) {}
