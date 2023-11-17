# *Slash*
### - Modernizing the shell while keeping the spirit from POSIX sh
*Slash* breaks free from the 30-year-old POSIX sh standard embracing a more intuitive and familiar language for shell scripting.

Although still in its infancy, Slash is already somewhat useful! The script used to format the source code is this simple Slash script.
```sh
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
./slash path_to_file
```

## Why a new shell language?
See [Slash's advances compared to POSIX sh](coming-soon).

Slash is mostly a toy language made for fun by its creator and should be understood accordingly. However, there are some genuine improvements that could be made to the shell. The POSIX shell has numerous syntax oddities, an outdated spec and some general quirky behavior (word splitting and quoting, types, error handling, etc.). Slash addresses some of these.

## A Brief Tour of Slash
The following is a small taste of what you can do using Slash.

In an interactive session, raw expressions are evaluated and printed out to the terminal. This means you can use Slash as a basic calculator without ever leaving the shell.
```sh
0xff + 1    # prints 256
0xff == 255 # prints 'true'
```
You want to do the same in POSIX sh? That would look something like:
```sh
echo $((0xff + 1))
echo [ $((0xff)) -eq 255 ] # prints 1 or 0
```

### C-style syntax without parentheses or semicolons
``` sh
var system = (uname)  # '(...)' is similar to the POSIX '$(...)' 
if $system == "Linux" {
    echo "well akthually youre using GNU/Linux!"
} elif $system == "Darwin" {
    echo "Charles"
}
```
```sh
var names = ["Alice", "Bob", "Callum"]
loop name in $names {
    if $name == "Callum" {
        echo "Hello, enemy"
    } else {
        echo "Hello, friend"
    }
}
```

### Types
Slash attempts to "keep all the good stuff" from POSIX sh while adding common language constructs found in general purpose scripting. One example of this is types. POSIX sh only has one type -- text.  Slash embraces types, and makes interpreting text as other data types trivial:
```sh
(ls | grep "*.png" | wc -l) as num >= 50 && echo "Many images!"
```

Slash has first-class support for dynamic lists, maps and tuples. These feel and behave similar to Python's list, dictionary and tuple.
```sh
var primes = [2, 3, 5, 7, 11, 13]
var some_primes = $primes[1..4]
var social_credit_score = @[ "brage": 10, "eilor": 300 ]  # map
var people = $social_credit_score.keys()  # returns a tuple: ("brage", "eilor")
```

### Want to know more?
A semi scuffed document containing the context free grammar can be found [here](https://github.com/LytixDev/slash/blob/main/docs/grammar.txt).

The lexer is ~600 lines, the parser is ~800 lines and the interpreter is around ~1000 lines. My goal is to keep the source code simple and hackable. The idea is that skilled programmer can understand and internalize the source code and make non trivial changes in an evening or so.  The source code contains **zero** third party code other than linking with a C11 standard library.


## Goals
- Fully featured interactive shell- and scripting language.
- Native shell features like `pipelines`, `I/O redirections`, `globbing` etc. 
- Modern language features, like `for-each loop`, `match` statement etc.
- Type system with powerful builtin types: `string`, `number`, `list`, `map`, `tuple`.
- Suitable for basic arithmetic and bitwise operations right an interactive session.
- Modern C-style syntax.
- Simple and hackable interpreter.
- Having fun writing code :-)

## Inspiration and Influences
1. Powerful and orthogonal builtin types like those found in Python
2. POSIX sh, of course.
3. Bash, Zsh and Fish.

The lexer takes heavy inspiration from [this talk](https://www.youtube.com/watch?v=HxaD_trXwRE) by Rob Pike. The rest of the interpreter takes inspiration from the book [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom.

## Contact
mail to nicolahb at stud dot ntnu dot no
