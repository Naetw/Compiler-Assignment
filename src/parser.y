%{
#include "AST/ast.hpp"
#include "AST/program.hpp"
#include "AST/decl.hpp"
#include "AST/variable.hpp"
#include "AST/ConstantValue.hpp"
#include "AST/function.hpp"
#include "AST/CompoundStatement.hpp"
#include "AST/print.hpp"
#include "AST/expression.hpp"
#include "AST/BinaryOperator.hpp"
#include "AST/UnaryOperator.hpp"
#include "AST/FunctionInvocation.hpp"
#include "AST/VariableReference.hpp"
#include "AST/assignment.hpp"
#include "AST/read.hpp"
#include "AST/if.hpp"
#include "AST/while.hpp"
#include "AST/for.hpp"
#include "AST/return.hpp"

#include "AST/constant.hpp"

#include "AST/AstDumper.hpp"

#include <cassert>
#include <errno.h>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define YYLTYPE yyltype

typedef struct YYLTYPE {
    uint32_t first_line;
    uint32_t first_column;
    uint32_t last_line;
    uint32_t last_column;
} yyltype;

extern int32_t line_num;  /* declared in scanner.l */
extern char buffer[];     /* declared in scanner.l */
extern FILE *yyin;        /* declared by lex */
extern char *yytext;      /* declared by lex */

static AstNode *root;

extern "C" int yylex(void); 
static void yyerror(const char *msg);
extern int yylex_destroy(void);
%}

%code requires {
    #include "AST/utils.hpp"
    #include "AST/PType.hpp"

    #include <vector>
    #include <memory>

    class AstNode;
    class DeclNode;
    class ConstantValueNode;
    class CompoundStatementNode;
}

    /* For yylval */
%union {
    /* basic semantic value */
    char *identifier;
    uint32_t integer;
    double real;
    char *string;
    bool boolean;

    int32_t sign;

    AstNode *node;
    PType *type_ptr;
    DeclNode *decl_ptr;
    CompoundStatementNode *compound_stmt_ptr;
    ConstantValueNode *constant_value_node_ptr;

    std::vector<std::unique_ptr<DeclNode>> *decls_ptr;
    std::vector<IdInfo> *ids_ptr;
    std::vector<uint64_t> *dimensions_ptr;
};

%type <identifier> ProgramName ID
%type <integer> INT_LITERAL
%type <real> REAL_LITERAL
%type <string> STRING_LITERAL
%type <boolean> TRUE FALSE
%type <sign> NegOrNot

%type <type_ptr> Type ScalarType ArrType
%type <decl_ptr> Declaration
%type <compound_stmt_ptr> CompoundStatement
%type <constant_value_node_ptr> LiteralConstant StringAndBoolean

%type <decls_ptr> DeclarationList Declarations
%type <ids_ptr> IdList
%type <dimensions_ptr> ArrDecl

    /* Follow the order in scanner.l */

    /* Delimiter */
%token COMMA SEMICOLON COLON
%token L_PARENTHESIS R_PARENTHESIS
%token L_BRACKET R_BRACKET

    /* Operator */
%token ASSIGN
%left OR
%left AND
%right NOT
%left LESS LESS_OR_EQUAL EQUAL GREATER GREATER_OR_EQUAL NOT_EQUAL
%left PLUS MINUS
%left MULTIPLY DIVIDE MOD
%right UNARY_MINUS

    /* Keyword */
%token ARRAY BOOLEAN INTEGER REAL STRING
%token END BEGIN_ /* Use BEGIN_ since BEGIN is a keyword in lex */
%token DO ELSE FOR IF THEN WHILE
%token DEF OF TO RETURN VAR
%token FALSE TRUE
%token PRINT READ

    /* Identifier */
%token ID

    /* Literal */
%token INT_LITERAL
%token REAL_LITERAL
%token STRING_LITERAL

%%

Program:
    ProgramName SEMICOLON
    /* ProgramBody */
    DeclarationList FunctionList CompoundStatement
    /* End of ProgramBody */
    END {
        root = new ProgramNode(@1.first_line, @1.first_column,
                               $1, new PType(PType::PrimitiveTypeEnum::kVoidType),
                               *$3, $5);

        free($1);
    }
;

ProgramName:
    ID
;

DeclarationList:
    Epsilon {
        $$ = new std::vector<std::unique_ptr<DeclNode>>();
    }
    |
    Declarations
;

Declarations:
    Declaration {
        $$ = new std::vector<std::unique_ptr<DeclNode>>();
        $$->emplace_back($1);
    }
    |
    Declarations Declaration {
        $1->emplace_back($2);
        $$ = $1;
    }
;

FunctionList:
    Epsilon
    |
    Functions
;

Functions:
    Function
    |
    Functions Function
;

Function:
    FunctionDeclaration
    |
    FunctionDefinition
;

FunctionDeclaration:
    FunctionName L_PARENTHESIS FormalArgList R_PARENTHESIS ReturnType SEMICOLON
;

FunctionDefinition:
    FunctionName L_PARENTHESIS FormalArgList R_PARENTHESIS ReturnType
    CompoundStatement
    END
;

FunctionName:
    ID
;

FormalArgList:
    Epsilon
    |
    FormalArgs
;

FormalArgs:
    FormalArg
    |
    FormalArgs SEMICOLON FormalArg
;

FormalArg:
    IdList COLON Type
;

IdList:
    ID {
        $$ = new std::vector<IdInfo>(); 
        $$->emplace_back(@1.first_line, @1.first_column, $1);
        free($1);
    }
    |
    IdList COMMA ID {
        $1->emplace_back(@3.first_line, @3.first_column, $3);
        free($3);
        $$ = $1;
    }
;

ReturnType:
    COLON ScalarType
    |
    Epsilon
;

    /*
       Data Types and Declarations
                                   */

Declaration:
    VAR IdList COLON Type SEMICOLON
    |
    VAR IdList COLON LiteralConstant SEMICOLON {
        $$ = new DeclNode(@1.first_line, @1.first_column, $2, $4);
        delete $2;
    }
;

Type:
    ScalarType
    |
    ArrType
;

    /* no need to release PType object since it'll be assigned to the shared_ptr */
ScalarType:
    INTEGER { $$ = new PType(PType::PrimitiveTypeEnum::kIntegerType); }
    |
    REAL { $$ = new PType(PType::PrimitiveTypeEnum::kRealType); }
    |
    STRING { $$ = new PType(PType::PrimitiveTypeEnum::kStringType); }
    |
    BOOLEAN { $$ = new PType(PType::PrimitiveTypeEnum::kBoolType); }
;

ArrType:
    ArrDecl ScalarType {
        $2->setDimensions(*$1);
        delete $1;
        $$ = $2;
    }
;

ArrDecl:
    ARRAY INT_LITERAL OF {
        $$ = new std::vector<uint64_t>{static_cast<uint64_t>($2)};
    }
    |
    ArrDecl ARRAY INT_LITERAL OF {
        $1->emplace_back(static_cast<uint64_t>($3));
        $$ = $1;
    }
;

LiteralConstant:
    NegOrNot INT_LITERAL {
        Constant::ConstantValue value;
        value.integer = static_cast<int64_t>($1) * static_cast<int64_t>($2);
        auto * const constant = new Constant(
            std::make_shared<PType>(
				PType::PrimitiveTypeEnum::kIntegerType),
            value);
        auto * const pos = ($1 == 1) ? &@2 : &@1;
        // no need to release constant object since it'll be assigned to the unique_ptr
        $$ = new ConstantValueNode(pos->first_line, pos->first_column, constant);
    }
    |
    NegOrNot REAL_LITERAL {
        Constant::ConstantValue value;
        value.real = static_cast<double>($1) * static_cast<double>($2);
        auto * const constant = new Constant(
            std::make_shared<PType>(
                PType::PrimitiveTypeEnum::kRealType),
            value);
        auto * const pos = ($1 == 1) ? &@2 : &@1;
        // no need to release constant object since it'll be assigned to the unique_ptr
        $$ = new ConstantValueNode(pos->first_line, pos->first_column, constant);
    }
    |
    StringAndBoolean
;

NegOrNot:
    Epsilon {
        $$ = 1;
    }
    |
    MINUS %prec UNARY_MINUS {
        $$ = -1;
    }
;

StringAndBoolean:
    STRING_LITERAL {
        Constant::ConstantValue value;
        value.string = $1;
        auto * const constant = new Constant(
            std::make_shared<PType>(
                PType::PrimitiveTypeEnum::kStringType),
            value);
        $$ = new ConstantValueNode(@1.first_line, @1.first_column, constant);
    }
    |
    TRUE {
        Constant::ConstantValue value;
        value.boolean = $1;
        auto * const constant = new Constant(
            std::make_shared<PType>(
                PType::PrimitiveTypeEnum::kBoolType),
            value);
        $$ = new ConstantValueNode(@1.first_line, @1.first_column, constant);
    }
    |
    FALSE {
        Constant::ConstantValue value;
        value.boolean = $1;
        auto * const constant = new Constant(
            std::make_shared<PType>(
                PType::PrimitiveTypeEnum::kBoolType),
            value);
        $$ = new ConstantValueNode(@1.first_line, @1.first_column, constant);
    }
;

IntegerAndReal:
    INT_LITERAL
    |
    REAL_LITERAL
;

    /*
       Statements
                  */

Statement:
    CompoundStatement
    |
    Simple
    |
    Condition
    |
    While
    |
    For
    |
    Return
    |
    FunctionCall
;

CompoundStatement:
    BEGIN_
    DeclarationList
    StatementList
    END {
        $$ = new CompoundStatementNode(@1.first_line, @1.first_column, *$2);
    }
;

Simple:
    VariableReference ASSIGN Expression SEMICOLON
    |
    PRINT Expression SEMICOLON
    |
    READ VariableReference SEMICOLON
;

VariableReference:
    ID ArrRefList
;

ArrRefList:
    Epsilon
    |
    ArrRefs
;

ArrRefs:
    L_BRACKET Expression R_BRACKET
    |
    ArrRefs L_BRACKET Expression R_BRACKET
;

Condition:
    IF Expression THEN
    CompoundStatement
    ElseOrNot
    END IF
;

ElseOrNot:
    ELSE
    CompoundStatement
    |
    Epsilon
;

While:
    WHILE Expression DO
    CompoundStatement
    END DO
;

For:
    FOR ID ASSIGN INT_LITERAL TO INT_LITERAL DO
    CompoundStatement
    END DO
;

Return:
    RETURN Expression SEMICOLON
;

FunctionCall:
    FunctionInvocation SEMICOLON
;

FunctionInvocation:
    ID L_PARENTHESIS ExpressionList R_PARENTHESIS
;

ExpressionList:
    Epsilon
    |
    Expressions
;

Expressions:
    Expression
    |
    Expressions COMMA Expression
;

StatementList:
    Epsilon
    |
    Statements
;

Statements:
    Statement
    |
    Statements Statement
;

Expression:
    L_PARENTHESIS Expression R_PARENTHESIS
    |
    MINUS Expression %prec UNARY_MINUS
    |
    Expression MULTIPLY Expression
    |
    Expression DIVIDE Expression
    |
    Expression MOD Expression
    |
    Expression PLUS Expression
    |
    Expression MINUS Expression
    |
    Expression LESS Expression
    |
    Expression LESS_OR_EQUAL Expression
    |
    Expression GREATER Expression
    |
    Expression GREATER_OR_EQUAL Expression
    |
    Expression EQUAL Expression
    |
    Expression NOT_EQUAL Expression
    |
    NOT Expression
    |
    Expression AND Expression
    |
    Expression OR Expression
    |
    IntegerAndReal
    |
    StringAndBoolean
    |
    VariableReference
    |
    FunctionInvocation
;

    /*
       misc
            */
Epsilon:
;

%%

void yyerror(const char *msg) {
    fprintf(stderr,
            "\n"
            "|-----------------------------------------------------------------"
            "---------\n"
            "| Error found in Line #%d: %s\n"
            "|\n"
            "| Unmatched token: %s\n"
            "|-----------------------------------------------------------------"
            "---------\n",
            line_num, buffer, yytext);
    exit(-1);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./parser <filename> [--dump-ast]\n");
        exit(-1);
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror("fopen() failed:");
    }

    yyparse();

    if (argc >= 3 && strcmp(argv[2], "--dump-ast") == 0) {
        AstDumper ast_dumper;
        root->accept(ast_dumper);
    }

    printf("\n"
           "|--------------------------------|\n"
           "|  There is no syntactic error!  |\n"
           "|--------------------------------|\n");

    delete root;
    fclose(yyin);
    yylex_destroy();
    return 0;
}
