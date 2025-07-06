Homework 3: Tiny PL/0 Compiler (Parser and Code Generator)
Sophia Kropivnitskaia and Harshika Jindal

Description: Implementation of parser and code generator for the programming language PL/0. Program gets tokens produced by Scanner(Lexical Analyzer) and produce, as output, if the program does not follow the grammar, a message indicating the type of error present.

Compilation Instructions:
gcc  parsercodegen.c -o lex
./lex < input.txt > output.txt

Usage:
The required argument is the input file, where the source program will be read. 

Example:
Input File:
var x, y;
begin
x:= y * 2;
end.

Output File:
               
Line 	 OP L  M
0 	JMP 0 13
1 	INC 0  5
2 	LOD 0  4
3 	LIT 0  2
4 	OPR 0  3
5 	STO 0  3
6 	SYS 0  3

Symbol Table:
Kind | Name | Value | Level | Address | Mark
---------------------------------------------------
2 | x | 0 | 0 | 3 | 1
2 | y | 0 | 0 | 4 | 1