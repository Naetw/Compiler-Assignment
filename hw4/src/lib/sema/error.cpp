#include "AST/ast.hpp"

#include <cstdarg>
#include <cstdio>

extern FILE *yyin;
extern long line_positions[];

void logSemanticError(const Location &p_location, const char *format, ...) {
    std::fprintf(stderr, "<Error> Found in line %u, column %u: ",
                 p_location.line, p_location.col);

    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);

    // print notation
    constexpr uint32_t kIndentionWidth = 4;
    if (std::fseek(yyin, line_positions[p_location.line], SEEK_SET) == 0) {
        char buffer[512];
        std::fgets(buffer, sizeof(buffer), yyin);
        std::fprintf(stderr, "\n%*s%s", kIndentionWidth, "", buffer);
        std::fprintf(stderr, "%*s\n", kIndentionWidth + p_location.col, "^");
    } else {
        std::fprintf(stderr, "Fail to reposition the yyin file stream.\n");
    }
}
