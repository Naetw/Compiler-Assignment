%{
#include <stdint.h>

#define LIST                concatenateString(yytext)
#define TOKEN(t)            { LIST; if (opt_tok) printf("<%s>\n", #t); }
#define TOKEN_CHAR(t)       { LIST; if (opt_tok) printf("<%c>\n", (t)); }
#define TOKEN_STRING(t, s)  { LIST; if (opt_tok) printf("<%s: %s>\n", #t, (s)); }
#define MAX_LINE_LENG       512

uint32_t line_num = 1;
static char buffer[MAX_LINE_LENG];

static uint32_t opt_src = 1;
static uint32_t opt_tok = 1;
static char string_literal[MAX_LINE_LENG];
static char *buffer_ptr = buffer;

static void concatenateString(const char *yytext_ptr);

%}

integer 0|[1-9][0-9]*
float {integer}\.(0|[0-9]*[1-9])

%%
    /* Delimiter */
"," { TOKEN_CHAR(','); }
";" { TOKEN_CHAR(';'); }
":" { TOKEN_CHAR(':'); }
"(" { TOKEN_CHAR('('); }
")" { TOKEN_CHAR(')'); }
"[" { TOKEN_CHAR('['); }
"]" { TOKEN_CHAR(']'); }

    /* Operator */
"+"   { TOKEN_CHAR('+'); }
"-"   { TOKEN_CHAR('-'); }
"*"   { TOKEN_CHAR('*'); }
"/"   { TOKEN_CHAR('/'); }
"mod" { TOKEN(mod); }
":="  { TOKEN(:=); }
"<"   { TOKEN_CHAR('<'); }
"<="  { TOKEN(<=); }
"<>"  { TOKEN(<>); }
">="  { TOKEN(>=); }
">"   { TOKEN_CHAR('>'); }
"="   { TOKEN_CHAR('='); }
"and" { TOKEN(and); }
"or"  { TOKEN(or); }
"not" { TOKEN(not); }

    /* Reserved Word */
"var"     { TOKEN(KWvar); }
"array"   { TOKEN(KWarray); }
"of"      { TOKEN(KWof); }
"boolean" { TOKEN(KWboolean); }
"integer" { TOKEN(KWinteger); }
"real"    { TOKEN(KWreal); }
"string"  { TOKEN(KWstring); }

"true"    { TOKEN(KWtrue); }
"false"   { TOKEN(KWfalse); }

"def"     { TOKEN(KWdef); }
"return"  { TOKEN(KWreturn); }

"begin"   { TOKEN(KWbegin); }
"end"     { TOKEN(KWend); }

"while"   { TOKEN(KWwhile); }
"do"      { TOKEN(KWdo); }

"if"      { TOKEN(KWif); }
"then"    { TOKEN(KWthen); }
"else"    { TOKEN(KWelse); }

"for"     { TOKEN(KWfor); }
"to"      { TOKEN(KWto); }

"print"   { TOKEN(KWprint); }
"read"    { TOKEN(KWread); }

    /* Identifier */
[a-zA-Z][a-zA-Z0-9]* { TOKEN_STRING(id, yytext); }

    /* Integer (decimal/octal) */
{integer} { TOKEN_STRING(integer, yytext); }
0[0-7]+   { TOKEN_STRING(oct_integer, yytext); }

    /* Floating-Point */
{float} { TOKEN_STRING(float, yytext); }

    /* Scientific Notation [Ee][+-]?[0-9]+ */
({integer}|{float})[Ee][+-]?({integer}) { TOKEN_STRING(scientific, yytext); }

    /* String */
\"([^"\n]|\"\")*\" {
    char *yyt_ptr = yytext + 1;  // +1 for skipping the first double quote "
    char *str_ptr = string_literal;

    while (*yyt_ptr) {
        if (*yyt_ptr == '"') {
            // Handle the situation of two double quotes "" in string literal
            if (*(yyt_ptr + 1) == '"') {
                *str_ptr = *yyt_ptr;
                ++str_ptr;
                yyt_ptr += 2; // move to the next character of ""
            } else {
                ++yyt_ptr;
            }
        } else {  // normal character
            *str_ptr = *yyt_ptr;
            ++str_ptr;
            ++yyt_ptr;
        }
    }
    *str_ptr = '\0';
    TOKEN_STRING(string, string_literal);
}

    /* Newline */
\n {
    if (opt_src) {
        printf("%d: %s\n", line_num, buffer);
    }
    ++line_num;
    buffer[0] = '\0';
    buffer_ptr = buffer;
}

    /* Catch the character which is not accepted by rules above */
. {
    printf("Error at line %d: bad character \"%s\"\n", line_num, yytext);
    exit(-1);
}

%%

static void concatenateString(const char *yytext_ptr) {
    while (*yytext_ptr) {
        *buffer_ptr = *yytext_ptr;
        ++buffer_ptr;
        ++yytext_ptr;
    }
    *buffer_ptr = '\0';
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./scanner [filename]\n");
        exit(-1);
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "Open file error\n");
        exit(-1);
    }

    yylex();

    return 0;
}
