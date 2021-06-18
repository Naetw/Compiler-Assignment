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

#include "sema/SemanticAnalyzer.hpp"
#include "codegen/CodeGenerator.hpp"

#include "AST/constant.hpp"
#include "AST/operator.hpp"

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
extern uint32_t opt_dmp;  /* declared in scanner.l */
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
    class FunctionNode;
    class ExpressionNode;
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
    FunctionNode *func_ptr;
    ExpressionNode *expr_ptr;

    std::vector<std::unique_ptr<DeclNode>> *decls_ptr;
    std::vector<IdInfo> *ids_ptr;
    std::vector<uint64_t> *dimensions_ptr;
    std::vector<std::unique_ptr<FunctionNode>> *funcs_ptr;
    std::vector<std::unique_ptr<AstNode>> *nodes_ptr;
    std::vector<std::unique_ptr<ExpressionNode>> *exprs_ptr;
};

%type <identifier> ProgramName ID FunctionName
%type <integer> INT_LITERAL
%type <real> REAL_LITERAL
%type <string> STRING_LITERAL
%type <boolean> TRUE FALSE
%type <sign> NegOrNot

%type <node> Statement Simple Condition While For Return FunctionCall
%type <type_ptr> Type ScalarType ArrType ReturnType
%type <decl_ptr> Declaration FormalArg
%type <compound_stmt_ptr> CompoundStatement ElseOrNot
%type <constant_value_node_ptr> LiteralConstant StringAndBoolean
%type <func_ptr> Function FunctionDeclaration FunctionDefinition
%type <expr_ptr> Expression IntegerAndReal FunctionInvocation VariableReference

%type <decls_ptr> DeclarationList Declarations FormalArgList FormalArgs
%type <ids_ptr> IdList
%type <dimensions_ptr> ArrDecl
%type <funcs_ptr> FunctionList Functions
%type <nodes_ptr> StatementList Statements
%type <exprs_ptr> ExpressionList Expressions ArrRefList ArrRefs

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
                               *$3, *$4, $5);

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
    Epsilon {
        $$ = new std::vector<std::unique_ptr<FunctionNode>>();
    }
    |
    Functions
;

Functions:
    Function {
        $$ = new std::vector<std::unique_ptr<FunctionNode>>();
        $$->emplace_back($1);
    }
    |
    Functions Function {
        $1->emplace_back($2);
        $$ = $1;
    }
;

Function:
    FunctionDeclaration
    |
    FunctionDefinition
;

FunctionDeclaration:
    FunctionName L_PARENTHESIS FormalArgList R_PARENTHESIS ReturnType SEMICOLON {
        $$ = new FunctionNode(@1.first_line, @1.first_column, $1, *$3, $5, nullptr);
        free($1);
        delete $3;
    }
;

FunctionDefinition:
    FunctionName L_PARENTHESIS FormalArgList R_PARENTHESIS ReturnType
    CompoundStatement
    END {
        $$ = new FunctionNode(@1.first_line, @1.first_column, $1, *$3, $5, $6);
        free($1);
        delete $3;
    }
;

FunctionName:
    ID
;

FormalArgList:
    Epsilon {
        $$ = new std::vector<std::unique_ptr<DeclNode>>();
    }
    |
    FormalArgs
;

FormalArgs:
    FormalArg {
        $$ = new std::vector<std::unique_ptr<DeclNode>>();
        $$->emplace_back($1);
    }
    |
    FormalArgs SEMICOLON FormalArg {
        $1->emplace_back($3);
        $$ = $1;
    }
;

FormalArg:
    IdList COLON Type {
        $$ = new DeclNode(@1.first_line, @1.first_column, $1, $3);
        delete $1;
    }
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
    COLON ScalarType {
        $$ = $2;
    }
    |
    Epsilon {
        $$ = new PType(PType::PrimitiveTypeEnum::kVoidType);
    }
;

    /*
       Data Types and Declarations
                                   */

Declaration:
    VAR IdList COLON Type SEMICOLON {
        $$ = new DeclNode(@1.first_line, @1.first_column, $2, $4);
        delete $2;
    }
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
    INT_LITERAL {
        Constant::ConstantValue value;
        value.integer = static_cast<int64_t>($1);
        auto * const constant = new Constant(
            std::make_shared<PType>(
				PType::PrimitiveTypeEnum::kIntegerType),
            value);
        // no need to release constant object since it'll be assigned to the unique_ptr
        $$ = new ConstantValueNode(@1.first_line, @1.first_column, constant);
    }
    |
    REAL_LITERAL {
        Constant::ConstantValue value;
        value.real = static_cast<double>($1);
        auto * const constant = new Constant(
            std::make_shared<PType>(
                PType::PrimitiveTypeEnum::kRealType),
            value);
        // no need to release constant object since it'll be assigned to the unique_ptr
        $$ = new ConstantValueNode(@1.first_line, @1.first_column, constant);
    }
;

    /*
       Statements
                  */

Statement:
    CompoundStatement {
        $$ = static_cast<AstNode *>($1);
    }
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
        $$ = new CompoundStatementNode(@1.first_line, @1.first_column,
                                       *$2, *$3);
    }
;

Simple:
    VariableReference ASSIGN Expression SEMICOLON {
        $$ = new AssignmentNode(@2.first_line, @2.first_column,
                                dynamic_cast<VariableReferenceNode *>($1), $3);
    }
    |
    PRINT Expression SEMICOLON {
        $$ = new PrintNode(@1.first_line, @1.first_column, $2);
    }
    |
    READ VariableReference SEMICOLON {
        $$ = new ReadNode(@1.first_line, @1.first_column,
                          dynamic_cast<VariableReferenceNode *>($2));
    }
;

VariableReference:
    ID ArrRefList {
        $$ = new VariableReferenceNode(@1.first_line, @1.first_column, $1, *$2);
        free($1);
        delete $2;
    }
;

ArrRefList:
    Epsilon {
        $$ = new std::vector<std::unique_ptr<ExpressionNode>>();
    }
    |
    ArrRefs
;

ArrRefs:
    L_BRACKET Expression R_BRACKET {
        $$ = new std::vector<std::unique_ptr<ExpressionNode>>();
        $$->emplace_back($2);
    }
    |
    ArrRefs L_BRACKET Expression R_BRACKET {
        $1->emplace_back($3);
        $$ = $1;
    }
;

Condition:
    IF Expression THEN
    CompoundStatement
    ElseOrNot
    END IF {
        $$ = new IfNode(@1.first_line, @1.first_column, $2, $4, $5);
    }
;

ElseOrNot:
    ELSE
    CompoundStatement {
        $$ = $2;
    }
    |
    Epsilon {
        $$ = nullptr;
    }
;

While:
    WHILE Expression DO
    CompoundStatement
    END DO {
        $$ = new WhileNode(@1.first_line, @1.first_column, $2, $4);
    }
;

For:
    FOR ID ASSIGN INT_LITERAL TO INT_LITERAL DO
    CompoundStatement
    END DO {
        Constant::ConstantValue value;
        Constant *constant;
        ConstantValueNode *constant_value_node;

        // DeclNode
        auto *ids = new std::vector<IdInfo>{IdInfo(@2.first_line, @2.first_column,
                                                   $2)};
        auto *type = new PType(PType::PrimitiveTypeEnum::kIntegerType);
        auto *var_decl = new DeclNode(@2.first_line, @2.first_column, ids, type);

        // AssignmentNode
        auto *var_ref = new VariableReferenceNode(@2.first_line, @2.first_column, $2);
        value.integer = static_cast<int64_t>($4);
        constant = new Constant(
            std::make_shared<PType>(PType::PrimitiveTypeEnum::kIntegerType),
            value);
        constant_value_node = new ConstantValueNode(@4.first_line, @4.first_column,
                                                    constant);
        auto *assignment = new AssignmentNode(@3.first_line, @3.first_column,
                                              var_ref, constant_value_node);

        // ExpressionNode
        value.integer = static_cast<int64_t>($6);
        constant = new Constant(
            std::make_shared<PType>(PType::PrimitiveTypeEnum::kIntegerType),
            value);
        constant_value_node = new ConstantValueNode(@6.first_line, @6.first_column,
                                                    constant);

        $$ = new ForNode(@1.first_line, @1.first_column,
                         var_decl, assignment, constant_value_node,
                         $8);
        free($2);
        delete ids;
    }
;

Return:
    RETURN Expression SEMICOLON {
        $$ = new ReturnNode(@1.first_line, @1.first_column, $2);
    }
;

FunctionCall:
    FunctionInvocation SEMICOLON
;

FunctionInvocation:
    ID L_PARENTHESIS ExpressionList R_PARENTHESIS {
        $$ = new FunctionInvocationNode(@1.first_line, @1.first_column, $1, *$3);
        free($1);
        delete $3;
    }
;

ExpressionList:
    Epsilon {
        $$ = new std::vector<std::unique_ptr<ExpressionNode>>();
    }
    |
    Expressions
;

Expressions:
    Expression {
        $$ = new std::vector<std::unique_ptr<ExpressionNode>>();
        $$->emplace_back($1);
    }
    |
    Expressions COMMA Expression {
        $1->emplace_back($3);
        $$ = $1;
    }
;

StatementList:
    Epsilon {
        $$ = new std::vector<std::unique_ptr<AstNode>>();
    }
    |
    Statements
;

Statements:
    Statement {
        $$ = new std::vector<std::unique_ptr<AstNode>>();
        $$->emplace_back($1);
    }
    |
    Statements Statement {
        $1->emplace_back($2);
        $$ = $1;
    }
;

Expression:
    L_PARENTHESIS Expression R_PARENTHESIS {
        $$ = $2;
    }
    |
    MINUS Expression %prec UNARY_MINUS {
        $$ = new UnaryOperatorNode(@1.first_line, @1.first_column,
                                   Operator::kNegOp, $2);
    }
    |
    Expression MULTIPLY Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kMultiplyOp, $1, $3);
    }
    |
    Expression DIVIDE Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kDivideOp, $1, $3);
    }
    |
    Expression MOD Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kModOp, $1, $3);
    }
    |
    Expression PLUS Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kPlusOp, $1, $3);
    }
    |
    Expression MINUS Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kMinusOp, $1, $3);
    }
    |
    Expression LESS Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kLessOp, $1, $3);
    }
    |
    Expression LESS_OR_EQUAL Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kLessOrEqualOp, $1, $3);
    }
    |
    Expression GREATER Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kGreaterOp, $1, $3);
    }
    |
    Expression GREATER_OR_EQUAL Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kGreaterOrEqualOp, $1, $3);
    }
    |
    Expression EQUAL Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kEqualOp, $1, $3);
    }
    |
    Expression NOT_EQUAL Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kNotEqualOp, $1, $3);
    }
    |
    NOT Expression {
        $$ = new UnaryOperatorNode(@1.first_line, @1.first_column,
                                   Operator::kNotOp, $2);
    }
    |
    Expression AND Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kAndOp, $1, $3);
    }
    |
    Expression OR Expression {
        $$ = new BinaryOperatorNode(@2.first_line, @2.first_column,
                                    Operator::kOrOp, $1, $3);
    }
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
        fprintf(stderr, "Usage: ./compiler <filename> --save-path [save path]\n");
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

    SemanticAnalyzer sema_analyzer(opt_dmp);
    root->accept(sema_analyzer);

    CodeGenerator code_generator(argv[1], (argc == 4) ? argv[3] : "",
                                 sema_analyzer.getSymbolManager());
    root->accept(code_generator);

    if (!sema_analyzer.hasError()) {
        printf("\n"
               "|---------------------------------------------------|\n"
               "|  There is no syntactic error and semantic error!  |\n"
               "|---------------------------------------------------|\n");
    }

    delete root;
    fclose(yyin);
    yylex_destroy();
    return 0;
}
