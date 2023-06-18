# Slash
Slash aims to be a very simple yet powerful interactive shell scripting language. It breaks free from shackles of the POSIX standard that was standardized some 30 years ago in favor of a more modern language design.

## Quick start
```sh
git clone --recurse-submodules git@github.com:LytixDev/slash.git
cd slash
make
./slash  # runs a default test.slash file
./slash path_to_file
```

## Design

Design document of the language can be found [here](https://github.com/LytixDev/slash/blob/main/docs/syntax.md). A semi scuffed document containing the context free grammar expression grammer can be found [here](https://github.com/LytixDev/slash/blob/main/docs/grammar.txt).

### REPL-like
Raw expressions are evaluted and printed out to the terminal.
```
0xff + 1    # prints 256
10 - -1     # prints 11
0xff == 255 # prints 'true'
```
You want to do the same in POSIX sh? That would look something like:
```sh
echo $((0xff + 1))  # prints 256
# or
if [ $((0xff)) -eq 255 ]; then
    echo "true"
else
    echo "false"
fi
```
This verbosity is really annoying and makes the POSIX shell a poor choice for quick mafs. One major goal for Slash is empowering (buzzy buzzy) the user to do simple and more complex calculations ergonomically (buzzy buzzy).
### C-style, but no parentheses or semicolons
``` 
var system = (uname)  # '(...)' is exactly the same as the POSIX '$(...)' 
if $system == "Linux" {
    echo "well akthually youre using GNU/Linux"
}
```
### Keeping the good stuff from POSIX sh while improving on the crap parts
```
(ls | wc -l) as num >= 50 && echo "Big directory!"
```

### Loop
```
loop c in "Alice" {
    echo $c
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
```
var primes = [2, 3, 5, 7, 11, 13]
$primes[1..4] # prints '[3, 5, 7]'
```

### Functions
```
func fib(n) {
    if $n <= 0 {
        return 0
    } elif $n == 1 {
        return 1
    } else {
        return fib(n=$n-1) + fib(n=$n-2)
    }
 }

var result = fib(n=5)
```

### Inspiration and Influences
C, Python, Go, Fish, Nushell and POSIX sh of course.

The lexer is heavily influenced by [this brilliant talk](https://www.youtube.com/watch?v=HxaD_trXwRE) by Rob Pike. The rest of the interpreter takes inspiration from the book [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom.

## Goals
- Functioning interactive shell.
- REPL.
- Simple yet powerful for basic calcuations.
- Modern language features, like `for-each-esque loop`.
- Modern syntax C-style syntax.
- More performant than the likes of BASH and ZSH (of course, these shells are POSIX compliant, so this is an apples to oranges type comparison)
- Simple and hackable interpreter.
- Have fun writing C code :-)

## Status
Currently variable assignment, basic arihtmetic, if-elif-else, loops, list, slices, ranges.
## Contact
mail to nicolahb at stud dot ntnu dot no
