# Project Assignment 2 - Syntactic Definitions

**Introduction to Compiler Design by Prof. Yi-Ping You**

### Due date: **23:59, 4/20, 2021**

In this assignment, you have to write an **LALR(1)** parser for the **`P`** language using `yacc`. This document provides the syntactic definitions of the `P` language. You should follow those syntactic definitions to implement the parser.

Once the LALR(1) grammar is defined, you can use `yacc` (or `bison`) to produce a C program, which is called `y.tab.c` (or `<input file>.tab.c`) by default. That program contains functions to perform the tasks defined by the LALR(1) grammar you wrote.

In `y.tab.c` (or `<input file>.tab.c`), `yyparse()` is the entry function of the parser. You are supposed to provide a `main` function that invokes `yyparse()` in your yacc source file, which has been done in the starter code. Since `yyparse()` calls `yylex()` to get a token scanned, you also have to modify your lex source file so that the scanner (`yylex()`) returns a toekn.

The course project has been divided into five incremental parts:

1. Implement a scanner (lexical analyzer) using `lex` (`flex`).
2. Implement a parser (syntax analyzer) using `yacc` (`bison`). (this assignment)
3. Augment the parser with the appropriate semantic actions to generate an **abstract syntax tree** (**AST**).
4. Augment the parser with appropriate semantic actions to perform a semantic analysis against the AST.
5. Augment the parser with appropriate semantic actions to generate RISC-V assembly code against the AST.

As you can see, each assignment is based on the previous one. Make sure the program you write is well-structured.

---

**Table of Contents**
- [Project Assignment 2 - Syntactic Definitions](#project-assignment-2---syntactic-definitions)
    - [Due Date:](#due-date)
  - [Syntactic Definitions](#syntactic-definitions)
    - [Program Units](#program-units)
      - [Program](#program)
      - [Function](#function)
    - [Data Types and Declarations](#data-types-and-declarations)
      - [Variable](#variable)
      - [Constant](#constant)
    - [Statements](#statements)
      - [Compound](#compound)
      - [Simple](#simple)
      - [Conditional](#conditional)
      - [While](#while)
      - [For](#for)
      - [Return](#return)
      - [Function Call](#function-call)
    - [Expressions](#expressions)
  - [Implementation Notes](#implementation-notes)
  - [What Should Your Parser Do?](#what-should-your-parser-do)
  - [Project Structure](#project-structure)
  - [Build and Execute](#build-and-execute)
    - [Build project](#build-project)
    - [Test your parser](#test-your-parser)
  - [Submitting the Assignment](#submitting-the-assignment)

---

## Syntactic Definitions

### Program Units

A program unit can be either a *program* or a *function*.

#### Program

A program has the following form:

    identifier;
    <zero or more variable and constant declaration>
    <zero or more function declaration and definition>
    compound statement
    end

A program has no arguments, and hence no parentheses are present in the header.

#### Function

A function can be a function declaration or a function definition.

A function declaration has the following forms:

    identifier (<zero or more formal arguments, separated by semicolons>): scalar_type;

or

    identifier (<zero or more formal arguments, separated by semicolons>);

A function definition has the following forms:

    identifier (<zero or more formal arguments, separated by semicolons>): scalar_type
    compound statement
    end

or

    identifier (<zero or more formal arguments, separated by semicolons>)
    compound statement
    end

Note that the parentheses after `identifier` are required even if no formal argument is declared.

A formal argument has the following form:

    identifier_list: type

where `identifier_list` is a list of identifiers separated by commas:

    identifier, identifier, ..., identifier

The `identifier_list` contains at least one identifier; therefore, in a formal argument, at least one identifier has to appear before a colon. The colon is followed by exactly one type (including array type) specification.

A function may return one value or nothing. Consequently, the return type declaration is either a type specification or empty. If the return type declaration is empty, the colon after the right parenthesis not required. A function that returns no value can be called a `"procedure"`. The following examples are all valid function declarations:

    func1(x, y: integer; z: string): boolean;
    func2(a: boolean): string;
    func3();        // procedure
    func4(b: real); // procedure
    func5(): integer;

### Data Types and Declarations

#### Variable

Four predefined scalar data types are `integer`, `real`, `string`, and `boolean`. The only structured type is the `array`. A variable declaration has the following forms:

    var identifier_list: scalar_type;

or

    var identifier_list: array integer_constant of type;

where `integer_constant` should be a non-negative integer constant that represents the size of the array.

#### Constant

A constant declaration has the following form:

    var identifier_list: literal_constant;

where `literal_constant` is a constant of the proper type (e.g., an integer/real literal with or without a negative sign, a string literal, true, or false).

### Statements

Statements can be one of the following statements: compound, simple, conditional, while, for, return, and function call.

#### Compound

A compound statement consists of a block of variable and constant declarations and a block of statements, and is delimited by the reserved words `begin` and `end`:

    begin
    <zero or more variable and constant declarations>
    <zero or more statements>
    end

#### Simple

A simple statement can be an assignment, a print statement, or a read statement.

An assignment has the following form:

    variable_reference := expression;

A print statement has the following form:

    print expression;

A read statement has the following form:

    read variable_reference;

where `variable_reference` can be simply an *identifier* or an *array_reference* in the form of

    identifier<zero or more expressions wrapped by brackets>

For example, both `arr[30]` and `arr[30][33]` are syntactically valid array references.

#### Conditional

A conditional statement has the following forms:

    if expression then
    compound statement
    else
    compound statement
    end if

or

    if exprression then
    compound statement
    end if

#### While

A while statement has the following form:

    while expression do
    compound statement
    end do

#### For

A for statement has the following form:

    for identifier := integer_constant to integer_constant do
    compound statement
    end do

#### Return

A return statement has the following form:

    return expression;

#### Function Call

A function call has the following form:

    identifier(<zero or more expressions, separated by commas>);

### Expressions

Expressions can be one of the following expressions: a literal constant, a variable reference, a function call without the semicolon, and an arithmetic expression.

Arithmetic expressions are written in infix notation, using the following operators with the precedence:

|Precedence|Operator|Description|
|:-:|:-:|:-:|
|1|**`-`**|negative|
|2|**`*`**|multiplication|
|3| **`/`**, **`mod`**|division and remainder|
|4|**`+`**|addition|
|5|**`-`**|subtraction|
|6| **`<`**, **`<=`**, **`<>`**, **`>=`**, **`>`**, **`=`** |relational|
|7|**`and`**, **`or`**, **`not`**|logical|

Note that:

1. The token **"-"** can be either the **binary subtraction** operator, or the **unary negation** operator.
2. Associativity is left. Parentheses may be used to group subexpressions to dictate a different precedence.
3. Semantic checking will be handled in the Assignment IV. In this assignment, you don't need to check semantic errors like **"a := 3 + true;"**. Just take care of syntactic errors.

## Implementation Notes

- `yyparse()` needs to use `yylex()` to obtain a token from the input source file. Therefore, you have to modify the lex source file to make the scanner pass token information to `yyparse()`. For example, when the scanner recognizes an identifier, the action should be like:

    ([A-Za-z])([A-Za-z0-9])*		{ tokenString("id", yytext); return ID; }
    /* Note that the symbol `ID` is defined by the yacc parser */

- Here is a general form of context-free grammar to describe grammar rule of `zero or more something` (e.g., a function signature contains zero or more formal arguments):

    argseq : /* empty */
           | argseq1
           ;

    argseq1 : arg
            | argseq1 ',' arg
            ;

- `Bison` will generate `xxx.h` when you build your yacc source file (`xxx.y`) with `-d` option. The `xxx.h` has the same purpose as `y.tab.h` mentioned in the lecture note.

## What Should Your Parser Do?

The parser should list information according to **opt_tok** and **opt_src** options (same as in Assignment I). If the input file is syntactically correct, output

    |--------------------------------|
    |  There is no syntactic error!  |
    |--------------------------------|

Once the parser finds a syntactic error, generate an error message in the form of

    |--------------------------------------------------------------------------
    | Error found in Line #<line number where the error occurs>: <source code of that line>
    |
    | Unmatched token: <the token that is not recognized>
    |--------------------------------------------------------------------------

## Project Structure

+ README.md
+ /src
    + Makefile
    + **`scanner.l`**
    + **`parser.y`**
+ /report
    + **`READMD.md`**

In this project, you have to modify `src/scanner.l`, extend `src/parser.y` (provided), and write your report in `report/README.md`.

> **Note**: You can extend your `scanner.l` from Assignment I rather than using `scanner.l` we provided.

The report should at least describe what changes you have made to your scanner and the abilities of your parser.

If you want to preview your report in GitHub style markdown before pushing to GitHub, [grip](https://github.com/joeyespo/grip) might be the tool you need.

## Build and Execute

+ Get HW2 docker image: `make docker-pull`
+ Activate docker environment: `./activate_docker.sh`
+ Build: `make`
+ Execute: `./parser <input file>`
+ Test: `make test`

### Build project

TA would use `src/Makefile` to build your project by simply typing `make clean && make`. You don't need to modify this file, but if you do, it is **your responsibility** to make sure this makefile have at least the same make targets we provided to you.

### Test your parser

We provide all the test cases in the `test` directory. Simply type `make test` to test your parser. The grade you got will be shown on the terminal. You can also check `diff.txt` in `test/result` directory to know the `diff` result between the outputs of your parser and the sample solutions.

Please using `student_` as the prefix of your own tests to prevent TAs from overriding your files. For example: `student_identifier_test`.

## Submitting the Assignment

You should push all your commits to the designated repository (hw2-\<Name of your GitHub account\>) under our GitHub organization by the deadline (given in the very beginning of this assignment description). At any point, you may save your work and push the repository. You **must** commit your final version to the **master branch**, and we will grade the commit which is **last pushed** on your master branch. The **push time** of that commit will be your submission time, so you **should not** push any commits to the master branch after the deadline if you have finished your assignment; otherwise, you will get a late penalty.

Note that the penalty for late homework is **15% per day** (weekends count as 1 day). Late homework will not be accepted after sample codes have been posted.

In addition, homework assignments **must be individual work**. If I detect what I consider to be intentional plagiarism in any assignment, the assignment will receive reduced or, usually, **zero credit**.
