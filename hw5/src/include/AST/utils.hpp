#ifndef AST_UTILS_H
#define AST_UTILS_H

#include "AST/ast.hpp"

#include <cstdint>
#include <string>

// for carrying identifier info through IdList
struct IdInfo {
    Location location;
    std::string id;

    IdInfo(const uint32_t line, const uint32_t col, const char * const p_id)
        : location(line, col), id(p_id) {}
};

#endif
