# hw2 report

|||
|-:|:-|
|Name|趙益揚|
|ID|0856058|

## How much time did you spend on this project

1-2 hours.

## Project overview

Describe the project structure and how you implemented it.

### Project structure

- 一份 lex 原始碼，沿用 hw1。裡面除了用 flex 的語法定義了 tokens 之外，也負責回傳解析到的 tokens 給 bison。
- 一份 yacc 原始碼，內容為根據作業說明並使用 bison 的語法定義了需要的語法文法。
- 原先提供的 Makefile，裡面使用 `flex`, `bison` 以及 `gcc` 等工具把 lex, yacc 原始碼編譯成一份執行檔。

### How I implement it

先將在 `scanner.l` 裡有定義的 tokens 在 `parser.y` 裡定義對應的 symbols 出來，這些 symbols 在經過 `bison` 編譯後會產生一個個 macros 並定義在 `parser.h` 裡。這邊使用到的 `bison` 語法是：

- `%token`
- `%left`：基本上就是 `%token` 但加上了 left-associativity 的性質。
- `%right`：基本上就是 `%token` 但加上了 right-associativity 的性質。

`scanner` 需要將解析到的 token 以 symbol 的形式回傳，`parser` 會負責呼叫請 `scanner` 回傳 token symbol。
在 `scanner.l` 裡引入 `parser.h`，這樣 `scanner` 才有各個 symbol 的定義，接著在各個 token 對應的語法中加入 `return <SOME_SYMBOL>;` 來做到回傳的任務。

最後就是依照作業說明上各個語法的結構在 `parser.y` 裡面用上面定義好的 symbols (terminal) 去定義並組合成各個語法結構 (non-terminal)。

## What is the hardest you think in this project

在定義語法時，自己寫的文法容易有衝突，需要透過 `parser.output` 裡面的資訊以及上課學過的技巧來把衝突解掉。

## Feedback to T.A.s

> Please help us improve our assignment, thanks.
