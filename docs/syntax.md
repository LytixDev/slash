# Types

Types are actually useful in SLASH unlike most shell scripting languages

## String (str)
`"string"`

TODO:
String buitins such as .lower, .upper, .split, slice?

## Number (num)
`3`
`3.14`
`0xff`
`0b0110`

## Range
`1..5`

## Shell Literal (shlit)
`ls`
`-la`

## Boolean (bool)
`true`
`false`


# Comments
```
# this is a comment
```


# Variables
Variables are defined using the `var` keyword. Prefix a variable with the '$' sign to get its value.

```
# assigns "Hello World!" to the variable a
var a = "Hello World!"

echo $a  # will print 'Hello World!'
```

# The `as` keyword
The `as` keyword can be used to convert between types.
```
var string1 = "123"
var string2 = 123 as str
var number1 = $string1 as num
var number2 = "123" as num
```


# Pipelines
The bread and butter of shell scripting. Pretty much identical to POSIX sh.
```
command1 | command2
```

```
ls | wc -l
```

# Comparison and Logical Operators
### Comparison
Pretty standard for programming language, but rare for shell scripting languages:

- `==` equals
- `!=` not-equals
- `<` less-than
- `<=` less-than-equal
- `>` greater-than
- `>=` greater-than-equal

POSIX sh does:
- `-eq` for equality (numeric comparison)
- `-ne` for not equal (numeric comparison)
- etc...
This is ugly, but had to be done since "<" and ">" where used for redirection. Slash takes a different approach by using the "<" and ">" characters for comparison.

### Logical Operators
The logical operators can be used in conjunction with any expression.
- `and`
- `or`
- `not`

```
var matches = $name == "Rodion" and $age < 30
```


# unfinished: Redirection
"<" and ">" are out of the picture as they are used for comparison. Instead, SLASH introduces the `rd` builtin command which replaces all redirections.

```
curl https://lytix.dev/ | rd index.html
```

is equivalent to the classic
```
curl https://lytix.dev/ > index.html
```

the common idiom
```
... > /dev/null
```

becomes
```
... | rd /dev/null
```


# unfinished: Mathematical Evaluation
Where SLASH shines (hopefully).
I want doing simple arithmetic and more "complex" math to feel super comfy and be really easy to do. I often open a python repl just to do basic maths. I want to be able to do math directly in the shell.
so far I think syntax should be:
```
(( math_expressions ))
```
The parsing rules inside a math expression will be somewhat different than the regular parsing rules. This will hopefully give flexibility to make the math expression as comfy as possible while also interfacing with the rest of the language.


# Subshell / Command Substitution
Statements inside parentheses are immediately evaluated. Pretty much identical to POSIX sh `$(...)`.
```
(command)
```

```
var file_count = (ls | wc -l) as num
```

```
var file_exists = (ls | grep "terry.*") as bool
```

```
echo "Current week number:" (date +"%W")
```


# Control Flow
Like any C-style language, but without parentheses around the conditional expression.

## if-else
```
if expr {
  stmt
}
```

```
var system = (uname)
if $system == "Linux" {
    echo "well akthually youre using GNU/Linux"
} 
```

```
var name = "Alice"

if $name == "Alice" or $name == "Bob" {
  echo "Hello, friend!"
} elif $name == "Callum" {
  echo "Hello, enemy!"
} else {
  echo "Hello, stranger!"
}
```

## and-if, else-if
Happily robbed from POSIX sh.
```
(ls | wc -l) as num >= 50 && echo "Big directory!"
```

```
false || echo "failed"
```


# Loop
The `loop` keyword can be used to create a classic for loop, a more modern "for-each" loop or the classic while loop.

The idiomatic for loop looks essentially like this:
```
loop VARIABLE in ITERABLE {
}
```

Here, the variable `i` will be defined only for the scope of the loop and its value will be inferred based on the iterable after the `in` keyword. This is the only place in the language where a variable can be defined without the `var` keyword.
```
loop i in 1..10 {
    echo $i  # prints: 1, 2, ... 9
}
```

```
loop c in "Alice" {
    echo $c  # prints A, l ... e
}
```

loop with condition essentially becomes a while loop
```
loop expr {
}
```
```
loop true {
}
```

Can also be used to create a sugar-free for loop.
```
var i = 0
loop $i < 10 {
  i += 1
}
```
