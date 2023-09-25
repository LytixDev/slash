# Types

```
# strings (str)
var str_t = "string"

# numbers (num)
var num_t = 3
var pi = 3.14
var hex = 0xff
var bin = 0b0110

# ranges (range)
var range_t = 1..5
var r = ..3

# boolean (bool)
var b = true

# list
var l = [1, 2, 3]
$l += 4
$l += [5, 6, 7]

var sliced = $l[..3]
var first = $l[0]

$l = [1, 2, "Baines"]  # A list can hold any data type

var popped = l.pop()  # every type also has builtin methods
$popped = l.pop(0)  # method overloading

# tuples
var t  = (1, 2, 3) # 1, 2, 3 without the parentheses is also valid

# map
var m = @["key": "value"]
```


# Variables
Variables are defined using the `var` keyword. Prefix a variable with the `$` sign to get its value. Using the `$` sign to grab a variables value is called a variable access in Slash-speak.
```
var a = "Hello World!"  # assigns "Hello World!" to the variable a
echo $a
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

# Comparison, Logical- and Arithmetic Operators
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

### Arithmetic operators

Arithmetic operators only work on numbers, other than the `+` operator which is also defined for lists and strings.

- `+` 
- `-`
- `*`
- `/`

```
var sum = 10 + 1.1	# adding to numbres
var sum2 = $sum - 0xff	# subtracting to numbers
```

```
var concat_str = "Hello " + "World!"
var concat_list = [1, 2, 3] + [4, 5, 6]
```

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
loop file in (ls) {
    echo $file
}
```

```
var names = ["Alice", "Bob"]
loop name in $names {
    echo $name
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
  $i += 1
}
```

# Functions

Functions are declared using the `func` keyword.
```
func say_hello() {
	echo "Hello!"
}

say_hello()
```

Functions can take in arbitrary many parameters. Arguments are not positional, but must be given to a function as keyword arguments. If only a variable interpolation is given as a function argument, its keyword argument is inferred based on the variable name.

```
func greet(name) {
	echo "Hello" $name
}

greet(name="Nicolai")
var name = "Alice"
greet($name)	# same as writing name = $name
# greet($name2) is not allowed as name2 can not be inferred as any keyword argument
```

```
func foo(a, b) {
	echo $a $b
}

func(a="Hello", b="World")
func(b="World", a="Hello")
```

A function can return any data using the return statement. If no return statement is present, the function returns nothing.
```
func fib(n) {
    if $n <= 0 {
        return 0
    } elif $n == 1 {
        return 1
    } else {
        return fibonacci(n=$n-1) + fibonacci(n=$n-2)
    }
 }

var result = fib(n=5)
```

