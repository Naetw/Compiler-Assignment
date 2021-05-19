#include "AST/PType.hpp"

const char *kTypeString[] = {"void", "integer", "real", "boolean", "string"};

// logical constness
const char *PType::getPTypeCString() const {
    if (!m_type_string_is_valid) {
        m_type_string += kTypeString[static_cast<size_t>(m_type)];

        if (m_dimensions.size() != 0) {
            m_type_string += " ";

            for (const auto &dim : m_dimensions) {
                m_type_string += "[" + std::to_string(dim) + "]";
            }
        }
        m_type_string_is_valid = true;
    }

    return m_type_string.c_str();
}

PType *PType::getStructElementType(const std::size_t nth) const {
    if (nth > m_dimensions.size()) {
        return nullptr;
    }

    auto *type_ptr = new PType(m_type);

    std::vector<uint64_t> dims;
    for (std::size_t i = nth; i < m_dimensions.size(); ++i) {
        dims.emplace_back(m_dimensions[i]);
    }
    type_ptr->setDimensions(dims);

    return type_ptr;
}
