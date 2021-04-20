%{
#include <stdint.h>

#include "parser.h"

#define LIST                strCat(yytext)
#define TOKEN(t)            { LIST; if (opt_tok) printf("<%s>\n", #t); }
#define TOKEN_CHAR(t)       { LIST; if (opt_tok) printf("<%c>\n", (t)); }
#define TOKEN_STRING(t, s)  { LIST; if (opt_tok) printf("<%s: %s>\n", #t, (s)); }
#define MAX_LINE_LENG       512

uint32_t line_num = 1;
char buffer[MAX_LINE_LENG];

static int32_t opt_src = 1;
static int32_t opt_tok = 1;
static char string_literal[MAX_LINE_LENG];
static char *buffer_ptr = buffer;

static void strCat(const char *Text);

%}

integer 0|[1-9][0-9]*
float {integer}\.(0|[0-9]*[1-9])

%x CCOMMENT

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

    /* Keyword */
"array"   { TOKEN(KWarray); }
"begin"   { TOKEN(KWbegin); }
"boolean" { TOKEN(KWboolean); }
"def"     { TOKEN(KWdef); }
"do"      { TOKEN(KWdo); }
"else"    { TOKEN(KWelse); }
"end"     { TOKEN(KWend); }
"false"   { TOKEN(KWfalse); }
"for"     { TOKEN(KWfor); }
"integer" { TOKEN(KWinteger); }
"if"      { TOKEN(KWif); }
"of"      { TOKEN(KWof); }
"print"   { TOKEN(KWprint); }
"read"    { TOKEN(KWread); }
"real"    { TOKEN(KWreal); }
"string"  { TOKEN(KWstring); }
"then"    { TOKEN(KWthen); }
"to"      { TOKEN(KWto); }
"true"    { TOKEN(KWtrue); }
"return"  { TOKEN(KWreturn); }
"var"     { TOKEN(KWvar); }
"while"   { TOKEN(KWwhile); }

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
    char *yyt_ptr = yytext;
    char *str_ptr = string_literal;

    /* Condition guard of the end of string literal
       Check only when *yyt_ptr == '"' */
    while (*(yyt_ptr + 1) != '\0') {
        ++yyt_ptr;

        /* Handle the situation of double quotes */
        if (*yyt_ptr == '"' && *(yyt_ptr + 1) != '\0')
            *str_ptr++ = *yyt_ptr++;

        while (*yyt_ptr != '"')
            *str_ptr++ = *yyt_ptr++;
    }
    *str_ptr = '\0';
    TOKEN_STRING(string, string_literal);
}
    /* Whitespace */
[ \t]+ { LIST; }

    /* Pseudocomment */
"//&"[ST][+-].* {
    LIST;
    char option = yytext[3];
    switch (option) {
    case 'S':
        opt_src = (yytext[4] == '+') ? 1 : 0;
        break;
    case 'T':
        opt_tok = (yytext[4] == '+') ? 1 : 0;
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

static void strCat(const char *Text) {
    while (*Text) {
        *buffer_ptr++ = *Text++;
    }
    *buffer_ptr = '\0';
}
