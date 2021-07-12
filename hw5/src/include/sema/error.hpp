#ifndef SEMA_ERROR_H
#define SEMA_ERROR_H

struct Location;

void logSemanticError(const Location &, const char *format, ...);

#endif
