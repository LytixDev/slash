# types

## string
`"string"`


## number
`3`
`3.14`

## word
`echo`
`ls`, `-la`

## range
`1..3`
`..10`


## boolean
`true`

# variables
```
var a = "Hello World!"
echo $a
```

# control flow

## if-else
```
if expr {
  stmt
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

## and if, or if
```
(ls | wc -l) as number -gt 5 && echo "There are more than 5 files in the directory"
```

```
false || echo "Command failed"
```

# loop
```
loop i in 1..10 {
  echo $i
}
```

```
loop 10 {
  # something
}
```

```
var i = 0
loop i -lt 10 {
  i += 1
}
```

# pipeline
```
command a | command b
```

# command substition
```
(command)
```

```
# this is a stupid way to check if a file exists btw
var file_exists = (ls | grep "terry.png") as boolean
```

```
echo (date)
```


# reserved

## keywords
var, if, elif, else, loop, in, true, false, as, and, or, string, number, boolean
