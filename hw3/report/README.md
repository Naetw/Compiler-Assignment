# hw3 report

|||
|-:|:-|
|Name|趙益揚|
|ID|0856058|

## How much time did you spend on this project

8.5 hours.

## Project overview

### Abstract Syntax Tree (AST)

基本上就照 sample code 的架構，新增幾個自己設計的類別來描述 P 語言所需特性：

#### 基礎類別 `AstNode`

作為所有節點的基礎類別，內部包含所有節點的共通特性：

- Location: 記錄節點與原始碼的行列數對應。
- accept(): 用來接受 visitor 並呼叫正確多載函式用。
- visitChildNodes(): 用來讓各個類別去把各自屬於節點類別的成員丟給 visitor。

#### `PType` & `Constant`

- 主要用來描述 P 語言的型別以及常數。
- 因型別與常數常會共用（例如多個變數都是 `integer` 型別），因此成員需要這兩種物件的類別都會以 C++ 的智慧指標 `shared_ptr` 來當容器，避免需要同時存在多個相同的物件。

### Visitor (`AstDumper`)

也是照著 sample code 的架構去補完 TODO 的缺口，缺口基本上就是各個節點類別所需要額外實做的一些資訊提供的 API，像是 `VariableNode` 需要提供變數名稱。

### `scanner.l`

對各個常數的正規表達式所對應的 C 程式中把字串轉成相對應的數值，儲存在 yylval 中。

### `parser.y`

- 對需要的 non-terminal 設定型別，透過：
	- `%code requires` 加入 yyltype 所需的型別宣告。
	- `%union` 把 yylval 需要的型別都加進去，包含 scanner 需要回傳的常數資料以及 non-terminal 會傳遞的物件型別。
	- `%type` 使用 `%union` 有宣告的型別來設定各個 non-terminal 的型別。
- 在 CFG 裡各個 non-terminals 加入 action 來建構 abstract syntax tree。
- yacc 遇到 epsilon 時並不會紀錄行列資訊，若有 epsilon 當作某個規則的開頭時需特別注意。
在 action 中把 yacc 紀錄好的行列資訊傳給各個節點的建構子。

## What is the hardest you think in this project

實做變數宣告 (`DeclNode`) 所需的準備較多：`PType`, `constant`, `VariableNode` 等等，寫起來量比較多才能夠動，稍嫌麻煩。這邊完成之後，後續的新增都比較偏勞力密集行為，依樣畫葫蘆即可。

Let us know, if any.

## Feedback to T.A.s

> Please help us improve our assignment, thanks.

visitor pattern 版本還是直接分開提供一份比較好，或是在 spec 裡加註要使用 visitor pattern 時所需的改動。
