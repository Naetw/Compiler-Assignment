#include "AST/constant.hpp"

static const char *kTFString[] = {"false", "true"};

// logical constness
const char *Constant::getConstantValueCString() const {
    if (!m_constant_value_string_is_valid) {
        switch (m_type->getPrimitiveType()) {
        case PType::PrimitiveTypeEnum::kIntegerType:
            m_constant_value_string = std::to_string(m_value.integer);
            break;
        case PType::PrimitiveTypeEnum::kRealType:
            m_constant_value_string = std::to_string(m_value.real);
            break;
        case PType::PrimitiveTypeEnum::kBoolType:
            m_constant_value_string = kTFString[m_value.boolean];
            break;
        case PType::PrimitiveTypeEnum::kStringType:
            m_constant_value_string = m_value.string;
            break;
        case PType::PrimitiveTypeEnum::kVoidType:
        default:
            break;
        }
        m_constant_value_string_is_valid = true;
    }
    return m_constant_value_string.c_str();
}
