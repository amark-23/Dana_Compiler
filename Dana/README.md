
# Dana Language Specifications

This document provides the full specifications of the **Dana** language, a simple imperative programming language. Below, you will find a detailed description of all language features, its lexical units, data types, program structure, expressions, statements, block structures, and the built-in library. A complete grammar in EBNF form is also included. 

You can also find some example programs in this directory:
   -bubblesort.dana    : the BubbleSort algorithm
   -hanoi.dana         : the well-known Towers of Hanoi problem
   -helloworld.dana    : typical hello world! program
   -primes.dana        : prints primes between 1 and n
   -reversestring.dana : reverses a string

---

## 1. Description of the Dana Language

The Dana language is a simple imperative programming language. Its main features, in brief, are as follows:

- Simple structure and syntax for statements and expressions that resemble those of Python.
- Basic data types for two sizes of integer numbers and arrays.
- Simple functions, with pass-by-value or pass-by-reference semantics.
- Variable scope rules similar to those in Pascal.
- A built-in function library.

More details about the language are provided in the paragraphs that follow.

---

### 1.1 Lexical Units

The lexical units (tokens) of the Dana language are divided into the following categories:

1. **Keywords**, which are:

   ```
   and, as, begin, break, byte, continue, decl,
   def, elif, else, end, exit, false, if,
   is, int, loop, not, or, ref, return,
   skip, true, var
   ```

2. **Names** (identifiers), which consist of a letter of the Latin alphabet, possibly followed by a sequence of letters, decimal digits, or underscore (`_`) characters. Names must not coincide with the keywords listed above. Uppercase and lowercase letters are considered different.

3. **Unsigned integer constants**, which consist of one or more decimal digits. Examples of integer constants:

   ```
   0     42     1284    00200
   ```

4. **Character constants**, which consist of a single character enclosed in single quotes. That character can be any common character or an escape sequence. Common characters include all printable characters except single quotes, double quotes, and the backslash (`\`). Escape sequences start with the backslash character (`\`) and are described in Table 1. Examples of character constants:

   ```
   'a'    '1'    '\n'    '\"'    '\x1d'
   ```

5. **String literals**, which consist of a sequence of common characters or escape sequences inside double quotes. String literals cannot span more than one program line. Examples of string literals:

   ```
   "abc"         "Route66"       "Helloworld!\n"
   "Name:\t\"DouglasAdams\"\nValue:\t42\n"
   ```

6. **Symbolic operators**, which are:

   ```
   +  -  *  /  %  !  &  |
   =  <>  <  >  <=  >=
   ```

7. **Separators**, which are:

   ```
   (  )  [  ]  ,  :  :=
   ```

In addition to the lexical units mentioned above, a Dana program may also include the following elements, which separate tokens and are ignored:

- **Whitespace characters**: sequences consisting of space characters, tab characters, line feed characters, or carriage return characters.
- **Single-line comments**, which begin with the `#` character and end at the end of the current line.
- **Multi-line comments**, which begin with the character sequence `(*` and end with the character sequence `*)`. These comments may be nested.

#### Table 1: Escape Sequences

| Escape Sequence | Description                                                |
|-----------------|------------------------------------------------------------|
| `\n`            | The line feed (newline) character                          |
| `\t`            | The tab character                                         |
| `\r`            | The carriage return character                             |
| `\0`            | The character with ASCII code 0                           |
| `\\`            | The `\` (backslash) character                             |
| `\'`            | The `'` (single quote) character                          |
| `\"`            | The `"` (double quote) character                          |
| `\xnn`          | The character with ASCII code `nn` in hexadecimal         |

---

### 1.2 Data Types

Dana supports two basic data types:

- **int**: integers at least 16 bits in size (range −32768 to 32767)
- **byte**: non-negative 8-bit integers (range 0 to 255)

Besides the basic types, Dana also supports **array types**, denoted `t [n]`. The `n` must be a positive integer constant, and `t` must be a valid type. Dana also implicitly supports a boolean-expression type, which is used only in `if` statement conditions. This should not be confused with the `byte` type, which may be used to declare variables storing logical (boolean) values.

---

### 1.3 Program Structure

Dana is a block-structured language. A program has, roughly, the same structure as a Pascal program. Blocks (or structural units) can be nested within each other, and the scope rules are the same as Pascal’s. The main program is a structural unit that does not return a result and does not accept parameters.

Each structural unit can optionally include:

- Variable declarations.
- Definitions of subprograms.
- Declarations of subprograms (whose definitions follow later).

The final component in the definition of a structural unit is its body, which is a block of statements (see Section 1.6).

---

#### 1.3.1 Variables

Variable declarations are introduced by the keyword `var`, followed by one or more variable names, the keyword `is`, and a data type. More consecutive variable declarations can be made by repeating the `var` keyword. Examples:

```plaintext
var i is int
var x y z is int
var s is byte [80]
```

---

#### 1.3.2 Structural Units

A structural unit is defined with the keyword `def`, followed by the unit’s **header**, local declarations, and its body. The header includes the name of the unit, an optional return type, and optional formal parameters. The return type is omitted for units that do not return a result (similar to procedures in Pascal). If a return type is present, it follows the keyword `is` and cannot be an array type. Formal parameters, if they exist, follow the separator `:` (colon), and their syntax resembles variable declarations except the keyword `as` replaces `is`.

Each formal parameter is characterized by its name, its type, and its passing mode. Dana supports **pass-by-value** (for basic types) and **pass-by-reference**. Pass-by-reference is indicated by the keyword `ref` preceding the parameter name if it is a basic type. All array-typed parameters are automatically passed by reference without using the `ref` keyword. In array types for formal parameters, the size of the first dimension can be omitted.

Examples of function/procedure headers:

```plaintext
def p1
def p2: n as int
def p3: a b as int, c as ref byte
def f1 is int: x as int
def f2 is int: s as byte []
def matrix_mult: a b c as int [10][10]
```

The local declarations of a structural unit follow its header. Dana follows Pascal’s scope rules regarding the visibility of variables, structural units, and parameters.

In the case of mutually recursive subprograms, a subprogram’s name must appear before its definition. In such a situation, to avoid violating scope rules, a **declaration** of the subprogram header must appear first (without its body). This is done with the keyword `decl` instead of `def`.

---

### 1.4 Expressions and Conditions

Each expression in Dana has a unique type and can be evaluated to yield a value of that type. Expressions are categorized into those that yield **l-values** (Section 1.4.1) and those that yield **r-values** (Sections 1.4.2 through 1.4.4). These names come from their position in an assignment statement: l-values appear on the left side, and r-values on the right side.

Dana’s **conditions** are described in Section 1.4.3. They are used only with the `if` statement, and their evaluation yields a boolean result (true or false).

Expressions and conditions can appear within parentheses for grouping.

---

#### 1.4.1 L-values

L-values represent objects that occupy memory at runtime and can contain values. Such objects include variables, subprogram parameters, array elements, and string literals. Specifically:

- The name of a variable or a parameter is an l-value and corresponds to that object. Its type is the same as that of the corresponding object.
- String literals, as described in Section 1.1, are also l-values. They have type `byte [n]`, where `n` is the number of characters in the string plus one. Each string literal corresponds to an array object containing the ASCII codes of its characters in sequence. At the end of the array, the character `'\0'` is automatically stored, following the C convention for null-terminated strings. String literals are the only kind of array constant allowed in Dana.
- If `l` is an l-value of type `t [m]` and `e` is an expression of type `int`, then `l[e]` is an l-value of type `t`. If the value of `e` is the non-negative integer `n`, this l-value corresponds to the array element at index `n` of the array represented by `l`. Array indices start at zero. The value of `n` must be within the actual bounds of the array (`n < m`).

When an l-value is used as an expression, its value is equal to the value contained in the corresponding object.

---

#### 1.4.2 Constants

Among the r-values in Dana, the following constants are included:

- **Unsigned integer constants**, as described in Section 1.1. They have type `int`, and their value is the non-negative integer they represent.
- **Character constants**, as described in Section 1.1. They have type `byte`, and their value is the ASCII code of the represented character.
- The keywords `true` and `false`. They are equivalent to the character constants `'\x01'` and `'\0'`, respectively.

---

#### 1.4.3 Operators

Dana’s operators are categorized into **unary** (one operand) and **binary** (two operands). Unary operators precede their operand (prefix), while binary operators are written between their operands (infix). The evaluation of operands occurs from left to right. Binary operators always evaluate both operands, except for `and` and `or`, as discussed below.

| **Table 2: Precedence and Associativity of Dana Operators**      |
|------------------------------------------------------------------|
| **Operators** | **Description**           | **Operands** | **Position/Associativity**          |
|---------------|---------------------------|--------------|-------------------------------------|
| + - !         | Sign, logical NOT (for byte)   | 1            | prefix                              |
| * / % &       | Multiplicative operators | 2            | infix, left-associative             |
| + - \|        | Additive operators       | 2            | infix, left-associative             |
| = <> > < <= >=| Relational operators     | 2            | infix, no associativity             |
| not           | Logical negation (conditions) | 1      | prefix                              |
| and           | Logical AND (conditions) | 2            | infix, left-associative (short-circuit) |
| or            | Logical OR (conditions)  | 2            | infix, left-associative (short-circuit)  |

All Dana operators produce an r-value or a condition. They are described in detail below:

- **Unary + and -** implement sign operators. The operand must be an `int` expression, and the result is an r-value of the same type.
- **Unary ! and not** implement logical negation.  
  - The `!` operator applies to expressions of type `byte`, and the result is an r-value of type `byte`.  
  - The `not` operator applies to conditions (boolean expressions) and produces a condition as a result.
- **Binary +, -, *, /, and %** implement arithmetic operations. The operands must be expressions of the same type (`int` or `byte`), and the result is an r-value of the same type.
- **Binary relational operators =, <>, <, >, <=, >=** compare two numbers. The operands must be expressions of the same type (`int` or `byte`), and the result is a condition (true or false).
- **Binary & and |** implement bitwise/boolean conjunction and disjunction for expressions of type `byte`.  
  - `&` yields `false` if any operand is `false`; otherwise, it yields `true`.  
  - `|` yields `false` only if both operands are `false`; otherwise, it yields `true`.  
  (In practice, these act as logical/boolean operations for bytes.)
- **Binary and and or** implement logical conjunction and disjunction for conditions. The operands must be conditions (boolean expressions), and the result is also a condition. The evaluation of these conditions uses **short-circuiting**:  
  - For `and`, if the first operand is false, the second operand is not evaluated.  
  - For `or`, if the first operand is true, the second operand is not evaluated.

In Table 2 above, the higher rows contain operators with higher precedence. Operators in the same row share the same precedence.

---

#### 1.4.4 Calling Structural Units as Functions

If `f` is the name of a structural unit with a return type `t`, then `f(e1, …, en)` is an r-value of type `t`. The number of actual parameters `n` must match the number of formal parameters of `f`. Also, the type and passing mode of each actual parameter must match those of the corresponding formal parameter, according to these rules:

- If the formal parameter is of type `t` and is passed by value, the corresponding actual parameter must be an expression of type `t`.
- If the formal parameter is of type `t` and is passed by reference, the corresponding actual parameter must be an l-value of type `t`.

When a structural unit is called, the actual parameters are evaluated from left to right.

---

### 1.5 Statements

Dana supports the following statements:

1. The **empty statement** `skip`, which does nothing.
2. The **assignment statement** `ℓ := e`, which assigns the value of expression `e` to the l-value `ℓ`. The l-value `ℓ` must be of some type `t` that is not an array, and the expression `e` must be of the same type `t`.
3. The **structural unit call statement**. If the unit accepts no parameters, this is done by writing its name. Otherwise, the actual parameters follow the separator `:` (colon), separated by commas. The same rules for matching parameters apply as in Section 1.4.4, except that the structural unit must not have a return type.
4. The **conditional statement**:

   ```
   if c1 : b1
   elif c2 : b2
   else : b3
   ```

   - The `elif` part can be repeated zero or more times.
   - The `else` part is optional.
   - `c1` and `c2` must be valid conditions.
   - `b1`, `b2`, and `b3` must be valid statement blocks (see Section 1.6).
   - Its semantics are the same as in Python.

5. The **loop statement** `loop i : b`. The identifier `i` is optional: if present, it is used as the name of the loop and follows Pascal’s scoping rules. `b` must be a valid statement block. The loop executes block `b` indefinitely, unless it is terminated by a `break`, `exit`, or `return` statement.
6. The **break statement** `break : i`, which terminates the execution of loop `i`. This statement must appear within the body of loop `i`. The separator `:` (colon) and the identifier `i` are optional. If they are absent, the innermost loop is terminated.
7. The **continue statement** `continue : i`, which restarts execution of loop `i` from its beginning. This statement must appear within the body of loop `i`. The separator `:` (colon) and the identifier `i` are optional. If they are absent, the innermost loop restarts.
8. The **exit statement** `exit`, which terminates the execution of the current structural unit. This statement must appear within the body of a structural unit that has no return type.
9. The **return statement** `return : e`, which terminates the execution of the current structural unit and returns the value of expression `e`. This statement must appear within the body of a structural unit that has a return type `t`. The expression `e` must have the same type `t`.

---

### 1.6 Blocks of Statements and the Layout Mechanism

Blocks of statements appear in Dana programs in three places: in the bodies of structural units, in the `if` statement, and in the `loop` statement. They consist of one or more statements and come in two possible forms:

1. **Explicit block with `begin` and `end`:**  
   A block of one or more statements enclosed by the keywords `begin` and `end`, for example:

   ```plaintext
   def fact is int: n as int
   begin
     if n <= 1:
       begin
         return: 0
       end
     else:
       begin
         return: n * fact(n-1)
       end
   end
   ```

   Note that `begin` and `end` are mandatory even if they enclose only one statement. Also note that no separator is required between consecutive statements.

2. **Layout-based block (Off-side Rule):**  
   To avoid the keywords `begin` and `end`, Dana provides a second form of block, supported by the **layout mechanism** (also known as the **off-side rule**), first introduced in the ISWIM language and used by Python and Haskell. Dana uses a simpler off-side rule than Python or Haskell:

   1. For each block of statements, we look for the immediately preceding lexical unit among `def`, `if`, `elif`, `else`, or `loop`. This token is called the **leader**.  
      (In the example above, the leaders for the three blocks are `def`, `if`, and `else`.)
   2. A block of statements that does not start with `begin` ends at the first lexical unit<sup>1</sup> that:  
      (a) is the first token in its line (preceded only by whitespace and/or comments), and  
      (b) is not indented more to the right than the leader.  
   3. At the end of the program, all open blocks of this form are implicitly closed.

   According to the off-side rule, the code above could equivalently be written as:

   ```plaintext
   def fact is int: n as int
   if n <= 1: return: 0
   else: return: n * fact(n-1)
   ```

   Here, the block of the `def` includes the second and third lines, both of which are indented more than the leader. The block of the `if` statement ends at the `else` (same indentation as the leader). The blocks of the `else` and `def` end at the end of the program.

   <sup>1</sup>Whitespace characters, e.g., spaces or comments, are not considered tokens.

For more and more complex examples, pay close attention to the indentation of the programs in Section 3 (if provided). All these examples use blocks only in this second form.

---

### 1.7 Built-in Library Functions

Dana supports a set of predefined structural units implemented in x86 assembly as a runtime library. They are visible in every structural unit, unless they are shadowed by variables, parameters, or other structural units with the same name. Below are their declarations and explanations.

---

#### 1.7.1 Input and Output

```plaintext
decl writeInteger: n as int
decl writeByte: b as byte
decl writeChar: b as byte
decl writeString: s as byte []
```

These functions print values of the basic Dana types, as well as print strings. `writeByte` and `writeChar` differ in that the former prints the numeric value of the byte, while the latter prints the character corresponding to that ASCII code.

```plaintext
decl readInteger is int
decl readByte is byte
decl readChar is byte
decl readString: n as int, s as byte []
```

These are used for input. `readString` reads a string up to the next newline character. Its parameters specify the maximum number of characters (including the final `'\0'`) to be read, and the byte array where they will be stored. The newline character is not stored. If the array size is exhausted before a newline appears, the reading will continue later from where it stopped.

---

#### 1.7.2 Conversion Functions

```plaintext
decl extend is int: b as byte
decl shrink is byte: i as int
```

- `extend` converts a `byte` to the corresponding `int`.
- `shrink` returns a `byte` containing the 8 least significant bits of its `int` parameter.

---

#### 1.7.3 String Handling Functions

```plaintext
decl strlen is int: s as byte []
decl strcmp is int: s1 s2 as byte []
decl strcpy: trg src as byte []
decl strcat: trg src as byte []
```

These functions behave exactly like their namesakes in the C standard library (`strlen`, `strcmp`, `strcpy`, and `strcat`).

---

## 2. Complete Grammar of Dana

The syntax of the Dana language is given below in EBNF form. The grammar that follows is ambiguous, but most ambiguities can be resolved by considering the precedence and associativity rules for operators (see Table 2). The symbols `<id>`, `<int-const>`, `<char-const>`, `<string-literal>`, and `<auto-end>` are terminal symbols of the grammar. The symbol `<auto-end>` is automatically inserted by the lexical analyzer based on the layout mechanism (see Section 1.6).

```ebnf
<program>         ::= <func-def>

<func-def>        ::= "def" <header> { <local-def> } <block>

<header>          ::= <id> [ "is" <data-type> ] [ ":" <fpar-def> { "," <fpar-def> } ]

<fpar-def>        ::= ( <id> )+ "as" <fpar-type>

<data-type>       ::= "int" | "byte"

<type>            ::= <data-type> { "[" <int-const> "]" }

<fpar-type>       ::= <type> 
                     | "ref" <data-type> 
                     | <data-type> "[" "]" { "[" <int-const> "]" }

<local-def>       ::= <func-def> | <func-decl> | <var-def>

<func-decl>       ::= "decl" <header>

<var-def>         ::= "var" ( <id> )+ "is" <type>

<stmt>            ::= "skip"
                     | <l-value> ":=" <expr>
                     | <proc-call>
                     | "exit"
                     | "return" ":" <expr>
                     | "if" <cond> ":" <block> { "elif" <cond> ":" <block> } [ "else" ":" <block> ]
                     | "loop" [ <id> ] ":" <block>
                     | "break" [ ":" <id> ]
                     | "continue" [ ":" <id> ]

<block>           ::= "begin" { <stmt> } "end"
                     | { <stmt> } <auto-end>

<proc-call>       ::= <id> [ ":" <expr> { "," <expr> } ]

<func-call>       ::= <id> "(" [ <expr> { "," <expr> } ] ")"

<l-value>         ::= <id>
                     | <string-literal>
                     | <l-value> "[" <expr> "]"

<expr>            ::= <int-const>
                     | <char-const>
                     | <l-value>
                     | "(" <expr> ")"
                     | <func-call>
                     | ( "+" | "-" ) <expr>
                     | <expr> ( "+" | "-" | "*" | "/" | "%" ) <expr>
                     | "true" | "false"
                     | "!" <expr>
                     | <expr> ( "&" | "|" ) <expr>

<cond>            ::= <expr>
                     | "(" <cond> ")"
                     | "not" <cond>
                     | <cond> ( "and" | "or" ) <cond>
                     | <expr> ( "=" | "<>" | "<" | ">" | "<=" | ">=" ) <expr>
```

*Note: Whitespace characters (such as spaces, tabs, or comments) do not count as separate tokens (`<auto-end>` is triggered by layout rules).*

---

### End of Specifications

This concludes the Dana language specifications. All of the above text is provided to give a complete overview of Dana’s syntax, semantics, and the built-in library.
```
