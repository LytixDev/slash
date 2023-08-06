# *Slash*
### - Modernizing the shell while keeping the good stuff from POSIX sh
*Slash* breaks free from the 30-year-old POSIX standard embracing a more intuitive and familiar language design while retaining goodies that makes the shell one of most valuable tools.

Although still in its infancy, *Slash* is already "somewhat" useful! The script used to format the source code for this project is this simple *Slash* script:
```
#!/bin/slash
loop file in (find -name "*.c" -o -name "*.h") {
    clang-format -i -style=file $file
}
```

## Quick start
```sh
git clone --recurse-submodules git@github.com:LytixDev/slash.git
cd slash
make
./slash  # runs a default test.slash file
./slash path_to_file
```

## Why a new shell script?
### Where the POSIX shell fail:
1. Syntax oddities =>hard to pick up.
2. Outdated spec => simple scripting idioms become unnecessarily complicated.

As a consequence of `1` and `2`, POSIX shell scripts tend to become complicated, error prone and hard to maintain.

### Why not Python or Perl?
In short; too clunky to communicate with the OS. Invoking and combining programs, piping, redirection and globbing is tedious and unergonomic.

### Why *Slash*?
*Slash* is a shell language with all the powerful elements of POSIX sh sprinkled with the necessary elements from general purpose scripting languages to enable the creation of **simple**, **effective** and **maintainable** shell scripts.

## A Brief Tour of *Slash*
Raw expressions are evaluated and printed out to the terminal.
```
0xff + 1    # prints 256
0xff == 255 # prints 'true'
```
You want to do the same in POSIX sh? That would look something like:
```sh
echo $((0xff + 1))
[ $((0xff)) -eq 255 ] && echo "true" || echo "false"
```

### C-style, but no parentheses or semicolons
``` 
var system = (uname)  # '(...)' is similar to the POSIX '$(...)' 
if $system == "Linux" {
    echo "well akthually youre using GNU/Linux"
}
```

### "Keeping the good stuff from POSIX sh while improving on the crap parts"
Unlike the POSIX shell, *Slash* makes interpreting text as other data types trivial:
```
(ls | grep "*.png" | wc -l) as num >= 50 && echo "Many images!"
```

### Modern Looping
```
loop file in (ls) {
    echo $file
}
```
```
loop i in 3..5 {
    echo $i
}
```
```
var names = ["Alice", "Bob", "Callum"]
loop name in $names {
    if $name == "Callum" {
        echo "Hello, enemy"
    } else {
        echo "Hello, friend"
    }
}
```

### First class lists, maps and tuples
```
# list
var primes = [2, 3, 5, 7, 11, 13]
# list from slice: [3, 5, 7]
var some_primes = $primes[1..4]
# map
var social_credit_score = @[ "brage": 10, "eilor": 300 ]
# tuple: ' "brage", "eilor" '
var people = $social_credit_score.keys()
```

### Want to know more?
A semi scuffed document containing the context free grammar can be found [here](https://github.com/LytixDev/slash/blob/main/docs/grammar.txt).

The lexer is ~500 lines, the parser is around ~600 lines and the interpreter is ~800 lines. My goal is to keep the source code simple and hackable. I propose, without any evidence, that a skilled programmer can understand and internalize the source code in an hour or so.  The source code contains **zero** third party code other than the C standard library itself. 


## Goals
- Fully featured interactive shell- and scripting language.
- Invaluable shell features like `pipelines`, `I/O redirections`, `globbing` etc. 
- Modern language features, like `for-each-esque loop`, `match` statement etc.
- Powerful builtin types and data structures: `string`, `number`, `list`, `map`, `tuple`.
- Powerful for basic arithmetic and bitwise operations.
- Modern C-style syntax.
- Simple and hackable interpreter.
- Having fun writing code :-)

## Inspiration and Influences
1. Powerful and orthogonal builtin types like Python
2. The delights from POSIX sh without the jank.


The lexer is heavily influenced by [this talk](https://www.youtube.com/watch?v=HxaD_trXwRE) by Rob Pike. The rest of the interpreter takes inspiration from the book [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom.

## Contact
mail to nicolahb at stud dot ntnu dot no
