#ifndef AST_P_TYPE_H
#define AST_P_TYPE_H

#include <memory>
#include <string>
#include <vector>

class PType;

using PTypeSharedPtr = std::shared_ptr<PType>;

class PType {
  public:
    enum class PrimitiveTypeEnum : uint8_t {
        kVoidType,
        kIntegerType,
        kRealType,
        kBoolType,
        kStringType
    };

  private:
    PrimitiveTypeEnum m_type;
    std::vector<uint64_t> m_dimensions;
    mutable std::string m_type_string;
    mutable bool m_type_string_is_valid = false;

  public:
    ~PType() = default;
    PType(const PrimitiveTypeEnum type) : m_type(type) {}

    void setDimensions(std::vector<uint64_t> &p_dims) {
        m_dimensions = std::move(p_dims);
    }

    PrimitiveTypeEnum getPrimitiveType() const { return m_type; }
    const char *getPTypeCString() const;

    const std::vector<uint64_t> &getDimensions() const { return m_dimensions; }

    PType *getStructElementType(const std::size_t nth) const;

    bool isPrimitiveInteger() const {
        return m_type == PrimitiveTypeEnum::kIntegerType;
    }
    bool isPrimitiveReal() const {
        return m_type == PrimitiveTypeEnum::kRealType;
    }
    bool isPrimitiveBool() const {
        return m_type == PrimitiveTypeEnum::kBoolType;
    }
    bool isPrimitiveString() const {
        return m_type == PrimitiveTypeEnum::kStringType;
    }

    bool isInteger() const {
        return isPrimitiveInteger() && m_dimensions.empty();
    }
    bool isReal() const { return isPrimitiveReal() && m_dimensions.empty(); }
    bool isBool() const { return isPrimitiveBool() && m_dimensions.empty(); }
    bool isString() const {
        return isPrimitiveString() && m_dimensions.empty();
    }
    bool isVoid() const {
        return m_type == PrimitiveTypeEnum::kVoidType && m_dimensions.empty();
    }

    bool isScalar() const {
        return m_dimensions.empty() && m_type != PrimitiveTypeEnum::kVoidType;
    }

    bool compare(const PType *p_type) const;
};

#endif
