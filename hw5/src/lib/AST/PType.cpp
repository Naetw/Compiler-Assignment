#include "AST/PType.hpp"

#include <cassert>

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

bool PType::compare(const PType *p_type) const {
    // primitive type comparison
    switch(m_type) {
    case PrimitiveTypeEnum::kIntegerType:
    case PrimitiveTypeEnum::kRealType:
        if (!p_type->isPrimitiveInteger() && !p_type->isPrimitiveReal()) {
            return false;
        }
        break;
    case PrimitiveTypeEnum::kBoolType:
        if (!p_type->isPrimitiveBool()) {
            return false;
        }
        break;
    case PrimitiveTypeEnum::kStringType:
        if (!p_type->isPrimitiveString()) {
            return false;
        }
        break;
    default:
        assert(false && "comparing unknown primitive type or void type");
        return false;
    }

    // dimensions comparison
    auto &dimensions = p_type->getDimensions();
    if (m_dimensions.size() != dimensions.size()) {
        return false;
    }
    for (decltype(m_dimensions)::size_type i = 0; i < m_dimensions.size(); ++i) {
        if (m_dimensions[i] != dimensions[i]) {
            return false;
        }
    }

    return true;
}
