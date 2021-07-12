# hw1 report

|Field|Value|
|-:|:-|
|Name|趙益揚|
|ID|0856058|

## How much time did you spend on this project

1 hour.

## Project overview

Describe the project structure and how you implemented it.

### Project structure

- 一份 lex 原始碼，內容為根據作業說明並使用 flex 的語法定義了需要的 tokens。
- 原先提供的 Makefile，裡面使用 `flex` 以及 `gcc` 工具把 lex 原始碼編譯成一份執行檔。

### How I implement it

Delimiter, operator, reserved word 基本上都是格式固定的 token，只要原封不動用 `"<text>"` 去作為對應的 regular expression 並在 action 的區塊使用適當的巨集去把 token 輸出即可。

Identifier, integer, floating-point, scientific notation 則是稍微複雜一點，需要用一些 regular expression 特殊的輔助語法描述。一樣設計好 regular expression 後再在 action 的區塊使用適當的巨集去把 token 輸出。

String 除了要使用特別一點的 regex 語法輔助外，在 action 的地方還要特別處理：需要把頭尾的雙引號 (`"`) 給丟掉，以及把被當成字串本身內容的雙引號 (`""`) 的其中一個給丟掉，這樣透過巨集輸出的字串內容才會是作業說明所要求的字串格式。

Whitespace, pseudocomment, C++ style comment 也是簡單用一些 regex 的語法來描述即可。唯一特別的是，pseudocomment 需要根據比對到的內容去把相對應的 flag 開關給打開或關閉。

最後則是 C style comment，為了設計方便，使用了 `flex` 李提供的一項功能 start condition，這項功能可以讓某些規則在特定的 condition (state) 下才被啟用。

作法是在碰到 `/*` 後就進入 `CCOMMENT` 這個 condition，然後 `<CCOMMENT>.` 這個規則就會把所有字元都給無視，直到遇到 `*/` 才把 condition 設回初始的狀態。

Scanner 本身就有一個 `<INITIAL>` condition，在 definitions 的區塊可以定義新的 condition，語法有 `%s` 跟 `%x` [1] 兩種。

我使用的是 `%x`，兩者差異是 inclusive 以及 exclusive，使用 exclusive 可以讓沒有特別指定 condition 的規則不會被自動套用，一定要特別在規則前面加上 `<CCOMMENT>` 的規則才會是在那個 condition 下會被啟用的規則。

## What is the hardest you think in this project

String action 的部分需要思考一下怎麼處理比較乾淨。

## Feedback to T.A.s

該修另一份 C- 的作業了吧。

## Reference

- [1]: https://westes.github.io/flex/manual/Start-Conditions.html#Start-Conditions
