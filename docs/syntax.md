# types

Types are actually useful in SLASH unlike most shell scripting languages

## string (str)
`"string"`

buitins such as .lower, .upper, .split
slice?

## number (num)
`3`
`3.14`

## range
`1..3`
`..10`

## shell literal (shlit)
`echo`
`ls`, `-la`

## boolean (bool)
`true`


# variables
```
# assigns "Hello World!" to the variable a
var a = "Hello World!"

# prints out the value at a
echo $a
```


# comments
```
# this is a comment
```


# pipeline
The bread and butter of shell scripting
```
command1 | command2
```


# comparison
Pretty standard for programming language, but rare for shell scripting langugages:

- == equals
- != not-equals
- < less-than
- <= less-than-equal
- > greater-than
- >= greater-than-equal

POSIX sh does:
- -eq for equality (numeric comparison)
- -ne for not equal (numeric comparison)

etc ... this is ugly, but had to be done since "<" and ">" where used for redirection


# redirection
"<" and ">" are out of the picture as they are used for comparison

```
curl https://lytix.dev/ | send index.html
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
... | send /dev/null
```


# mathematical evaluation
this is kind of the big selling point of SLASH.
I want doing simple arithmetic and more "complex" math to feel super comfy and be really easy to do.
I often open a python repl just to do basic maths. I want to be able to do math directly in the shell.
so far I think syntax should be:

```
(( math_expressions ))
```
the parsing rules inside a math expression will be somewhat different than the regular parsin rules.
this will hopefully give flexibility to make the math expression as comfy as possible while also
interfacing with the rest of the language.


# command substition
statements inside parantheses are immidiately evaluated
```
(command)
```

```
var file_count = (ls | wc -l) as num
```

```
var file_exists = (ls | grep "terry.png") as bool
```

```
echo "Current week number:" (date +"%W")
```


# control flow

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
```
(ls | wc -l) >= 50 && echo "Big directory"
```

```
false || echo "failed"
```


# loop
idiomatic for loop
essentially:
```
loop VARIABLE in ITERABLE {
}
```

```
loop i in 1..10 {
    echo $i
}

loop c in "Alice" {
    echo $c
}
```

loop with condition
```
loop EXPRESSION {
}
```

```
var i = 0
loop i > 10 {
  i += 1
}
```

