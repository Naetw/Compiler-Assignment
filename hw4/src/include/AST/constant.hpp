#ifndef AST_CONSTANT_H
#define AST_CONSTANT_H

#include "AST/PType.hpp"

#include <cstdint>
#include <cstdlib>

class Constant {
  public:
    union ConstantValue {
        int64_t integer;
        double real;
        char *string;
        bool boolean;
    };

  private:
    PTypeSharedPtr m_type;
    ConstantValue m_value;
    mutable std::string m_constant_value_string;
    mutable bool m_constant_value_string_is_valid = false;

  public:
    ~Constant() {
        if (m_type->getPrimitiveType() ==
            PType::PrimitiveTypeEnum::kStringType) {
            free(m_value.string);
        }
    }
    Constant(const PTypeSharedPtr &p_type, const ConstantValue value)
        : m_type(p_type), m_value(value) {}

    const PType *getTypePtr() const { return m_type.get(); }
    const PTypeSharedPtr &getTypeSharedPtr() const { return m_type; }
    const char *getConstantValueCString() const;

    decltype(m_value.integer) integer() const { return m_value.integer; }
};

#endif
