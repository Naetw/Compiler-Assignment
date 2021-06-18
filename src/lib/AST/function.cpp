#include "AST/function.hpp"
#include "AST/decl.hpp"

#include <algorithm>

FunctionNode::DeclNodes::size_type
FunctionNode::getParametersNum(const DeclNodes &p_parameters) {
    FunctionNode::DeclNodes::size_type num = 0;

    for (const auto &decl_node : p_parameters) {
        num += decl_node->getVariables().size();
    }

    return num;
}

std::string
FunctionNode::getParametersTypeString(const DeclNodes &p_parameters) {
    std::string type_string;

    for (const auto &parameter : p_parameters) {
        for (const auto &var_node : parameter->getVariables()) {
            type_string.append(var_node->getTypeCString()).append(", ");
        }
    }

    if (!p_parameters.empty()) {
        // remove trailing ", "
        type_string.erase(type_string.end() - 2, type_string.end());
    }

    return type_string;
}

const char *FunctionNode::getPrototypeCString() const {
    if (!m_prototype_string_is_valid) {
        m_prototype_string = m_ret_type->getPTypeCString();

        m_prototype_string += " (";
        m_prototype_string += getParametersTypeString(m_parameters);
        m_prototype_string += ")";

        m_prototype_string_is_valid = true;
    }

    return m_prototype_string.c_str();
}

void FunctionNode::visitChildNodes(AstNodeVisitor &p_visitor) {
    auto visit_ast_node = [&](auto &ast_node) { ast_node->accept(p_visitor); };

    for_each(m_parameters.begin(), m_parameters.end(), visit_ast_node);

    if (m_body) {
        visit_ast_node(m_body);
    }
}

void FunctionNode::visitBodyChildNodes(AstNodeVisitor &p_visitor) {
    if (m_body) {
        m_body->visitChildNodes(p_visitor);
    }
}
