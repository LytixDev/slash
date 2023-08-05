## Lexical Tokens
Slash source code is lexed into tokens separated by whitespace and newlines. A token can therefore never span across multiple lines.

## Special Tokens
( ) [ ] { } * ~ \ , : ; ' & && = == | || ! !! < <= > >= . .. + += - -= @ @[ $

## Reserved Keywords
var func return if elif else loop in true false as and or not str num bool list tuple map none assert break continue

## Other Tokens

numbers: Can start with 0x (base16), 0b (base2) or nothing (base10) denoting the base. Then a set of number characters valid for the particular base. Base ten numbers can also have a decimal point.

string: "" any characters enclosed inside double-qoutes

identifiers: Must start with a letter (a, b, .. z) or underscore (_). Then can be followed by any letter, number character (0, 1, .. 9) underscore (_) or hyphen  (-).

shell identifiers: Shell identifiers are less strict than regular identifiers. This is meant to loosly follow the POSIX standard of splitting identifiers. NOTE: not implemented.

comment: # consumes any character until newline
