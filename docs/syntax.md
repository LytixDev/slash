## Lexical Tokens
Slash source code is lexed into tokens separated by whitespace, tabs and newlines. A token can therefore never span across multiple lines.

## Special Tokens
( ) [ ] { } * ~ \ , : ; ' & && = == | || ! !! < <= > >= . .. + += - -= @ @[ $

## Reserved Keywords
var func return if elif else loop in true false as and or not str num bool list tuple map none assert break continue

## Other Tokens

numbers: Can start with 0x (base16), 0b (base2) or nothing (base10) denoting the base. Then a set of number characters valid for the particular base in addition to '-' as a visual seperator. Base ten numbers can also have a decimal point.

string:
    - "" any characters enclosed inside double-qoutes. Hanldes escape sequences (for example \n becomes newline).
    - '' any characters enclosed inside single-qoutes Does not hanlde escape sequences.

identifiers: Must start with a letter (a, b, .. z), underscore (_) or hyphen (-). Can then be any sequence of the aformentioned characters plus any number character (0, 1, .. 9).

comment: # consumes any character until newline
