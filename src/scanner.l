%{
#include <stdint.h>
#include <string.h>

#include "parser.h"

#define YY_USER_ACTION \
    yylloc.first_line = line_num; \
    yylloc.first_column = col_num; \
    col_num += yyleng;

#define LIST                concatenateString(yytext)
#define TOKEN(t)            { LIST; if (opt_tok) printf("<%s>\n", #t); }
#define TOKEN_CHAR(t)       { LIST; if (opt_tok) printf("<%c>\n", (t)); }
#define TOKEN_STRING(t, s)  { LIST; if (opt_tok) printf("<%s: %s>\n", #t, (s)); }
#define MAX_LINE_LENG       512
#define MAX_ID_LENG         32
#define MAX_LINE_NUM        200

// prevent undefined reference error in newer version of flex
extern "C" int yylex(void);

uint32_t line_num = 1;
uint32_t col_num = 1;
long line_positions[MAX_LINE_NUM + 1] = {0}; // +1 since we use 1-based
char buffer[MAX_LINE_LENG];

static uint32_t opt_src = 1;
static uint32_t opt_tok = 1;
uint32_t opt_dmp = 1;
static char string_literal[MAX_LINE_LENG];
static char *buffer_ptr = buffer;

static void concatenateString(const char *yytext_ptr);

%}

integer 0|[1-9][0-9]*
float {integer}\.(0|[0-9]*[1-9])

    /* exclusive start condition */
%x CCOMMENT

%%
    /* Delimiter */
"," { TOKEN_CHAR(','); return COMMA; }
";" { TOKEN_CHAR(';'); return SEMICOLON; }
":" { TOKEN_CHAR(':'); return COLON; }
"(" { TOKEN_CHAR('('); return L_PARENTHESIS; }
")" { TOKEN_CHAR(')'); return R_PARENTHESIS; }
"[" { TOKEN_CHAR('['); return L_BRACKET; }
"]" { TOKEN_CHAR(']'); return R_BRACKET; }

    /* Operator */
"+"   { TOKEN_CHAR('+'); return PLUS; }
"-"   { TOKEN_CHAR('-'); return MINUS; }
"*"   { TOKEN_CHAR('*'); return MULTIPLY; }
"/"   { TOKEN_CHAR('/'); return DIVIDE; }
"mod" { TOKEN(mod); return MOD; }
":="  { TOKEN(:=); return ASSIGN; }
"<"   { TOKEN_CHAR('<'); return LESS; }
"<="  { TOKEN(<=); return LESS_OR_EQUAL; }
"<>"  { TOKEN(<>); return NOT_EQUAL; }
">="  { TOKEN(>=); return GREATER_OR_EQUAL; }
">"   { TOKEN_CHAR('>'); return GREATER; }
"="   { TOKEN_CHAR('='); return EQUAL; }
"and" { TOKEN(and); return AND; }
"or"  { TOKEN(or); return OR; }
"not" { TOKEN(not); return NOT; }

    /* Reserved Word */
"var"     { TOKEN(KWvar); return VAR; }
"array"   { TOKEN(KWarray); return ARRAY; }
"of"      { TOKEN(KWof); return OF; }
"boolean" { TOKEN(KWboolean); return BOOLEAN; }
"integer" { TOKEN(KWinteger); return INTEGER; }
"real"    { TOKEN(KWreal); return REAL; }
"string"  { TOKEN(KWstring); return STRING; }

"true"    {
    TOKEN(KWtrue);
    yylval.boolean = true;
    return TRUE;
}
"false"   {
    TOKEN(KWfalse);
    yylval.boolean = false;
    return FALSE;
}

"def"     { TOKEN(KWdef); return DEF; }
"return"  { TOKEN(KWreturn); return RETURN; }

"begin"   { TOKEN(KWbegin); return BEGIN_; }
"end"     { TOKEN(KWend); return END; }

"while"   { TOKEN(KWwhile); return WHILE; }
"do"      { TOKEN(KWdo); return DO; }

"if"      { TOKEN(KWif); return IF; }
"then"    { TOKEN(KWthen); return THEN; }
"else"    { TOKEN(KWelse); return ELSE; }

"for"     { TOKEN(KWfor); return FOR; }
"to"      { TOKEN(KWto); return TO; }

"print"   { TOKEN(KWprint); return PRINT; }
"read"    { TOKEN(KWread); return READ; }

    /* Identifier */
[a-zA-Z][a-zA-Z0-9]* {
    TOKEN_STRING(id, yytext);
    yylval.identifier = strndup(yytext, MAX_ID_LENG);
    return ID;
}

    /* Integer (decimal/octal) */
{integer} {
    TOKEN_STRING(integer, yytext);
    yylval.integer = strtol(yytext, NULL, 10);
    return INT_LITERAL;
}
0[0-7]+   {
    TOKEN_STRING(oct_integer, yytext);
    yylval.integer = strtol(yytext, NULL, 8);
    return INT_LITERAL;
}

    /* Floating-Point */
{float} {
    TOKEN_STRING(float, yytext);
    yylval.real = atof(yytext);
    return REAL_LITERAL;
}

    /* Scientific Notation [Ee][+-]?[0-9]+ */
({integer}|{float})[Ee][+-]?({integer}) {
    TOKEN_STRING(scientific, yytext);
    yylval.real = atof(yytext);
    return REAL_LITERAL;
}

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
    yylval.string = strdup(string_literal);
    return STRING_LITERAL;
}

    /* Whitespace */
[ \t]+ { LIST; }

    /* Pseudocomment */
"//&"[STD][+-].* {
    LIST;
    char option = yytext[3];
    switch (option) {
    case 'S':
        opt_src = (yytext[4] == '+') ? 1 : 0;
        break;
    case 'T':
        opt_tok = (yytext[4] == '+') ? 1 : 0;
        break;
    case 'D':
        opt_dmp = (yytext[4] == '+') ? 1 : 0;
        break;
    }
}

    /* C++ Style Comment */
"//".* { LIST; }

    /* C Style Comment */
"/*"           { LIST; BEGIN(CCOMMENT); }
<CCOMMENT>"*/" { LIST; BEGIN(INITIAL); }
<CCOMMENT>.    { LIST; }

    /* Newline */
<INITIAL,CCOMMENT>\n {
    if (opt_src) {
        printf("%d: %s\n", line_num, buffer);
    }
    line_positions[line_num + 1] = line_positions[line_num] +
                                   (buffer_ptr - buffer + 1);
    ++line_num;
    col_num = 1;
    buffer[0] = '\0';
    buffer_ptr = buffer;
}

    /* Catch the character which is not accepted by all rules above */
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
