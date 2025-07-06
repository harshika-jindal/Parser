// Sophia Kropivnitskaia and Harshika Jindal
// Course: Systems Software
// Semester: Summer 2025
// HW 3 - Tiny PL/0 Compiler (Parser and Code Generator)

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_ID_LEN 11
#define MAX_NUM_LEN 5
#define MAX_LINE_LEN 256
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_TOKENS 10000
#define CODE_SIZE 500

// Token types
typedef enum {
    oddsym = 1, identsym = 2, numbersym = 3, plussym = 4, minussym = 5, 
    multsym = 6, slashsym = 7, fisym = 8, eqlsym = 9, neqsym = 10, lessym = 11, 
    leqsym = 12, gtrsym = 13, geqsym = 14, lparentsym = 15, rparentsym = 16, 
    commasym = 17, semicolonsym = 18, periodsym = 19, becomessym = 20, 
    beginsym = 21, endsym = 22, ifsym = 23, thensym = 24, whensym = 25, 
    dosym = 26, constsym = 27, varsym = 28, readsym = 29, writesym = 30
} tokenType;

// Token structure
typedef struct {
    int type;
    char lexeme[100];
    int line;
    int column;
} Token;

// Virtual machine instruction structure
typedef struct {
    int op;  // opcode
    int L;   // lexicographical level
    int M;   // modifier
} instruction;

// Symbol table structure
typedef struct {
    int kind;       // const = 1, var = 2
    char name[11];  // name up to 11 chars
    int val;        // number (ASCII value)
    int level;      // L level (always 0 for this assignment)
    int addr;       // M address
    int mark;       // to indicate unavailable or deleted
} symbol;

// Global variables
Token tokenList[MAX_TOKENS];
int tokenCount = 0;
int currentTokenIndex = 0;
Token* currentToken = NULL;

symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
int sym_table_size = 0;
instruction code[CODE_SIZE];
int cx = 0;  // code index

// Error handling
typedef struct {
    int line;
    int column;
    char message[100];
} Error;

Error errors[100];
int errorCount = 0;
int hasError = 0;

// Reserved words and symbols
// const char *reservedWords[] = {
//     "odd", "const", "var", "begin", "end", "if", "fi", "then", 
//     "when", "do", "read", "write"
// };

// const int reservedTokens[] = {
//     oddsym, constsym, varsym, beginsym, endsym, ifsym, fisym, thensym, 
//     whensym, dosym, readsym, writesym
// };

const char symbols[] = {
    '+', '-', '*', '/', '(', ')', '=', ',', '.', '<', '>', ';', ':'
};

const char* error_messages[] = {
    "program must end with period",
    "const, var, and read keywords must be followed by identifier",
    "symbol name has already been declared",
    "constants must be assigned with =",
    "constants must be assigned an integer value",
    "constant and variable declarations must be followed by a semicolon",
    "undeclared identifier",
    "only variable values may be altered",
    "assignment statements must use :=",
    "begin must be followed by end",
    "if must be followed by then",
    "when must be followed by do",
    "condition must contain comparison operator",
    "right parenthesis must follow left parenthesis",
    "arithmetic equations must contain operands, parentheses, numbers, or symbols",
    "then must be followed by fi",
    "unmatched right parenthesis",
    "constants must be integers (no decimal points)",
    "invalid symbol",
    "identifier too long",
    "number too long"
};

// Function prototypes
void printSourceProgram(FILE *input);
void scanTokens(FILE *input);
int isReservedWord(char* id);
void get_next_token();
void error(int error_num);
void emit(int op, int L, int M);
int find_symbol(char* name);
void program();
void block();
void const_declaration();
int var_declaration();
void statement();
void condition();
void expression();
void term();
void factor();
void print_errors();

// Helper functions
void add_error(int line, int col, const char* msg) {
    if (errorCount < 100) {
        errors[errorCount].line = line;
        errors[errorCount].column = col;
        strncpy(errors[errorCount].message, msg, 99);
        errorCount++;
        hasError = 1;
    }
}

int isLetter(char c) {
    return isalpha(c);
}

int isNumber(char c) {
    return isdigit(c);
}

int isASpecialSymbol(char input) {
    for (int i = 0; i < 13; i++) {
        if (input == symbols[i])
            return 1;
    }
    return 0;
}

// void printSourceProgram(FILE *input) {
//     char buffer[MAX_LINE_LEN];
//     printf("Source Program:\n");
//     rewind(input);
//     while (fgets(buffer, sizeof(buffer), input)) {
//         printf("%s", buffer);
//     }
//     rewind(input);
// }

int isReservedWord(char* id) {
    if (strcmp(id, "odd") == 0) return oddsym;
    if (strcmp(id, "const") == 0) return constsym;
    if (strcmp(id, "var") == 0) return varsym;
    if (strcmp(id, "begin") == 0) return beginsym;
    if (strcmp(id, "end") == 0) return endsym;
    if (strcmp(id, "if") == 0) return ifsym;
    if (strcmp(id, "then") == 0) return thensym;
    if (strcmp(id, "fi") == 0) return fisym;
    if (strcmp(id, "when") == 0) return whensym;  // Changed from "while"/whilesym
    if (strcmp(id, "do") == 0) return dosym;
    if (strcmp(id, "read") == 0) return readsym;
    if (strcmp(id, "write") == 0) return writesym;
    return 0; // not a reserved word
}

void scanTokens(FILE *input) {
    char buffer[MAX_LINE_LEN];
    int lineNum = 1;

    while (fgets(buffer, sizeof(buffer), input)) {
        int i = 0;
        int colNum = 1;
        while (buffer[i] != '\0') {
            char c = buffer[i];

            // Skip whitespace
            if (isspace(c)) { 
                i++; 
                colNum++;
                continue; 
            }

            // Handle comments
            if (buffer[i] == '/' && buffer[i + 1] == '*') {
                int j = i + 2;
                colNum += 2;
                while (!(buffer[j] == '*' && buffer[j + 1] == '/')) {
                    if (buffer[j] == '\0') {
                        add_error(lineNum, colNum, "Unterminated comment");
                        break;
                    }
                    j++;
                    colNum++;
                }
                i = j + 2;
                colNum += 2;
                continue;
            }

            // Process identifiers and reserved words
            if (isLetter(c)) {
                int startCol = colNum;
                char id[100];
                int j = 0;
                while (isLetter(buffer[i]) || isNumber(buffer[i])) {
                    if (j < 99) id[j++] = buffer[i];
                    i++;
                    colNum++;
                }
                id[j] = '\0';

                if (strlen(id) > MAX_ID_LEN) {
                    printf("%s\t\tError: Identifier too long\n", id);
                    add_error(lineNum, startCol, "Identifier too long");
                } else {
                    int token = isReservedWord(id);
                    if (token) {
                        printf("%s\t\t%d\n", id, token);
                    } else {
                        printf("%s\t\t%d\n", id, identsym);
                    }
                }
                continue;
            }

            // Process numbers
            if (isNumber(c)) {
                int startCol = colNum;
                char num[100];
                int j = 0;
                while (isNumber(buffer[i])) {
                    if (j < 99) num[j++] = buffer[i];
                    i++;
                    colNum++;
                }
                // Check for decimal point (invalid in PL/0)
                if (buffer[i] == '.') {
                    add_error(lineNum, colNum, "Decimal numbers not allowed");
                    while (isNumber(buffer[i]) || buffer[i] == '.') {
                        i++;
                        colNum++;
                    }
                    continue;
                }
                num[j] = '\0';
                if (strlen(num) > MAX_NUM_LEN) {
                    printf("%s\t\tError: Number too long\n", num);
                    add_error(lineNum, startCol, "Number too long");
                } else {
                    printf("%s\t\t%d\n", num, numbersym);
                }
                continue;
            }

            // Process special symbols
            switch (c) {
                case '+': printf("+\t\t%d\n", plussym); break;
                case '-': printf("-\t\t%d\n", minussym); break;
                case '*': printf("*\t\t%d\n", multsym); break;
                case '/': printf("/\t\t%d\n", slashsym); break;
                case '(': printf("(\t\t%d\n", lparentsym); break;
                case ')': printf(")\t\t%d\n", rparentsym); break;
                case '=': printf("=\t\t%d\n", eqlsym); break;
                case ',': printf(",\t\t%d\n", commasym); break;
                case '.': printf(".\t\t%d\n", periodsym); break;
                case '<':
                    if (buffer[i+1] == '=') { printf("<=\t\t%d\n", leqsym); i++; colNum++; }
                    else if (buffer[i+1] == '>') { printf("<>\t\t%d\n", neqsym); i++; colNum++; }
                    else { printf("<\t\t%d\n", lessym); }
                    break;
                case '>':
                    if (buffer[i+1] == '=') { printf(">=\t\t%d\n", geqsym); i++; colNum++; }
                    else { printf(">\t\t%d\n", gtrsym); }
                    break;
                case ';': printf(";\t\t%d\n", semicolonsym); break;
                case ':':
                    if (buffer[i+1] == '=') { printf(":=\t\t%d\n", becomessym); i++; colNum++; }
                    else { printf(":\t\tError: invalid symbol\n"); add_error(lineNum, colNum, "Invalid symbol ':'"); }
                    break;
                default:
                    printf("\t\tError: invalid symbol \"%c\"\n", c);
                    add_error(lineNum, colNum, "Invalid symbol");
                    break;
            }
            i++;
            colNum++;
        }
        lineNum++;
    }

    if (!hasError) {
        // Store tokens for parser
        rewind(input);
        lineNum = 1;
        while (fgets(buffer, sizeof(buffer), input)) {
            int i = 0;
            int colNum = 1;
            while (buffer[i] != '\0') {
                char c = buffer[i];

                if (isspace(c)) {
                    i++;
                    colNum++;
                    continue;
                }

                if (c == '/' && buffer[i+1] == '*') {
                    i += 2;
                    colNum += 2;
                    while (!(buffer[i] == '*' && buffer[i+1] == '/')) {
                        if (buffer[i] == '\0') break;
                        i++;
                        colNum++;
                    }
                    i += 2;
                    colNum += 2;
                    continue;
                }

                if (isLetter(c)) {
                    char id[100];
                    int j = 0;
                    int startCol = colNum;
                    while (isLetter(buffer[i]) || isNumber(buffer[i])) {
                        if (j < 99) id[j++] = buffer[i];
                        i++;
                        colNum++;
                    }
                    id[j] = '\0';
                    int token = isReservedWord(id);
                    tokenList[tokenCount].type = token ? token : identsym;
                    strcpy(tokenList[tokenCount].lexeme, token ? "" : id);
                    tokenList[tokenCount].line = lineNum;
                    tokenList[tokenCount].column = startCol;
                    tokenCount++;
                    continue;
                }

                if (isNumber(c)) {
                    char num[100];
                    int j = 0;
                    int startCol = colNum;
                    while (isNumber(buffer[i])) {
                        if (j < 99) num[j++] = buffer[i];
                        i++;
                        colNum++;
                    }
                    num[j] = '\0';
                    tokenList[tokenCount].type = numbersym;
                    strcpy(tokenList[tokenCount].lexeme, num);
                    tokenList[tokenCount].line = lineNum;
                    tokenList[tokenCount].column = startCol;
                    tokenCount++;
                    continue;
                }

                int singleToken = 0;
                switch (c) {
                    case '+': singleToken = plussym; break;
                    case '-': singleToken = minussym; break;
                    case '*': singleToken = multsym; break;
                    case '/': singleToken = slashsym; break;
                    case '(': singleToken = lparentsym; break;
                    case ')': singleToken = rparentsym; break;
                    case '=': singleToken = eqlsym; break;
                    case ',': singleToken = commasym; break;
                    case '.': singleToken = periodsym; break;
                    case '<':
                        if (buffer[i+1] == '=') { singleToken = leqsym; i++; colNum++; }
                        else if (buffer[i+1] == '>') { singleToken = neqsym; i++; colNum++; }
                        else { singleToken = lessym; }
                        break;
                    case '>':
                        if (buffer[i+1] == '=') { singleToken = geqsym; i++; colNum++; }
                        else { singleToken = gtrsym; }
                        break;
                    case ';': singleToken = semicolonsym; break;
                    case ':':
                        if (buffer[i+1] == '=') { singleToken = becomessym; i++; colNum++; }
                        break;
                }
                if (singleToken > 0) {
                    tokenList[tokenCount].type = singleToken;
                    tokenList[tokenCount].lexeme[0] = '\0';
                    tokenList[tokenCount].line = lineNum;
                    tokenList[tokenCount].column = colNum;
                    tokenCount++;
                }
                i++;
                colNum++;
            }
            lineNum++;
        }

        // Print token list
        printf("\nToken List:\n");
        for (int i = 0; i < tokenCount; i++) {
            if (tokenList[i].type == identsym || tokenList[i].type == numbersym)
                printf("%d %s ", tokenList[i].type, tokenList[i].lexeme);
            else
                printf("%d ", tokenList[i].type);
        }
        printf("\n");
    }
}

void get_next_token() {
    if (currentTokenIndex < tokenCount) {
        currentToken = &tokenList[currentTokenIndex++];
    } else {
        // End of tokens, treat as period
        static Token endToken = {periodsym, "", -1, -1};
        currentToken = &endToken;
    }
}

void error(int error_num) {
    if (error_num >= 0 && error_num < sizeof(error_messages)/sizeof(error_messages[0])) {
        if (currentToken->line != -1) {
            printf("Error (Line %d, Column %d): %s\n", 
                   currentToken->line, currentToken->column, error_messages[error_num]);
        } else {
            printf("Error: %s\n", error_messages[error_num]);
        }
    } else {
        printf("Unknown error\n");
    }
    exit(1);
}

void emit(int op, int L, int M) {
    if (cx >= CODE_SIZE) {
        error("Program too long");
    }
    code[cx].op = op;
    code[cx].L = L;
    code[cx].M = M;
    cx++;
}

// symbolTable Check
int find_symbol(char* name) {
    for (int i = 0; i < sym_table_size; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void program() {
    // First instruction should be JMP to skip variable allocation
    emit(JMP, 0, 13);
    
    get_next_token();
    block();
    if (currentToken->type != periodsym) {
        error(0); // program must end with period
    }
    
    // Mark all symbols at program end
    for (int i = 0; i < sym_table_size; i++) {
        symbol_table[i].mark = 1;
    }
    
    emit(SYS, 0, 3); // HALT instruction
}

void block() {
    const_declaration();
    int num_vars = var_declaration();
    emit(INC, 0, 3 + num_vars); // Allocate space for variables
    
    statement();
}

void const_declaration() {
    if (currentToken->type == constsym) {
        do {
            get_next_token();
            if (currentToken->type != identsym) {
                error(1); // const must be followed by identifier
            }
            
            char name[MAX_ID_LEN + 1];
            strcpy(name, currentToken->lexeme);
            
            if (find_symbol(name) != -1) {
                error(2); // symbol already declared
            }
            
            get_next_token();
            if (currentToken->type != eqlsym) {
                error(3); // constants must be assigned with =
            }
            
            get_next_token();
            if (currentToken->type != numbersym) {
                error(4); // constants must be assigned an integer value
            }
            
            // Additional check for decimal points
            if (strchr(currentToken->lexeme, '.') != NULL) {
                error(17); // constants must be integers
            }
            
            // Add to symbol table
            symbol_table[sym_table_size].kind = 1;
            strcpy(symbol_table[sym_table_size].name, name);
            symbol_table[sym_table_size].val = atoi(currentToken->lexeme);
            symbol_table[sym_table_size].level = 0;
            symbol_table[sym_table_size].addr = 0;
            symbol_table[sym_table_size].mark = 0;
            sym_table_size++;
            
            get_next_token();
        } while (currentToken->type == commasym);
        
        if (currentToken->type != semicolonsym) {
            error(5); // const declaration must end with semicolon
        }
        get_next_token();
    }
}

int var_declaration() {
    int num_vars = 0;
    if (currentToken->type == varsym) {
        do {
            num_vars++;
            get_next_token();
            if (currentToken->type != identsym) {
                error(1); // var must be followed by identifier
            }
            
            char name[MAX_ID_LEN + 1];
            strcpy(name, currentToken->lexeme);
            
            if (find_symbol(name) != -1) {
                error(2); // symbol already declared
            }
            
            // Add to symbol table
            symbol_table[sym_table_size].kind = 2;
            strcpy(symbol_table[sym_table_size].name, name);
            symbol_table[sym_table_size].val = 0;
            symbol_table[sym_table_size].level = 0;
            symbol_table[sym_table_size].addr = num_vars + 2; // First var at address 3
            symbol_table[sym_table_size].mark = 0;
            sym_table_size++;
            
            get_next_token();
        } while (currentToken->type == commasym);
        
        if (currentToken->type != semicolonsym) {
            error(5); // var declaration must end with semicolon
        }
        get_next_token();
    }
    return num_vars;
}

void statement() {
    if (currentToken->type == identsym) {
        // Assignment statement
        char name[MAX_ID_LEN + 1];
        strcpy(name, currentToken->lexeme);
        int sym_idx = find_symbol(name);
        
        if (sym_idx == -1) {
            error(6); // undeclared identifier
        }
        if (symbol_table[sym_idx].kind != 2) {
            error(7); // only variables can be assigned to
        }
        
        get_next_token();
        if (currentToken->type != becomessym) {
            error(8); // assignment must use :=
        }
        
        get_next_token();
        expression();
        emit(STO, 0, symbol_table[sym_idx].addr);
    }
    else if (currentToken->type == beginsym) {
        // Compound statement
        get_next_token();
        statement();
        
        while (currentToken->type == semicolonsym) {
            get_next_token();
            // Check if this is an empty statement before end
            if (currentToken->type != endsym) {
                statement();
            }
        }
        
        if (currentToken->type != endsym) {
            error(9); // begin must be followed by end
        }
        get_next_token();
    }
    else if (currentToken->type == ifsym) {
        // If statement
        get_next_token();
        condition();
        
        if (currentToken->type != thensym) {
            error(10); // if must be followed by then
        }
        
        int jpc_idx = cx;
        emit(JPC, 0, 0); // Placeholder address
        
        get_next_token();
        statement();
        
        if (currentToken->type != fisym) {
            error(15); // then must be followed by fi
        }
        get_next_token();
        
        code[jpc_idx].M = cx; // Update jump address
    }
    else if (currentToken->type == whensym) {
        // When (while) statement
        int loop_idx = cx;
        get_next_token();
        condition();
        
        if (currentToken->type != dosym) {
            error(11); // when must be followed by do
        }
        
        int jpc_idx = cx;
        emit(JPC, 0, 0); // Placeholder address
        
        get_next_token();
        statement();
        
        emit(JMP, 0, loop_idx);
        code[jpc_idx].M = cx; // Update jump address
    }
    else if (currentToken->type == readsym) {
        // Read statement
        get_next_token();
        if (currentToken->type != identsym) {
            error(1); // read must be followed by identifier
        }
        
        int sym_idx = find_symbol(currentToken->lexeme);
        if (sym_idx == -1) {
            error(6); // undeclared identifier
        }
        if (symbol_table[sym_idx].kind != 2) {
            error(7); // only variables can be read into
        }
        
        emit(SYS, 0, 1); // READ
        emit(STO, 0, symbol_table[sym_idx].addr);
        
        get_next_token();
    }
    else if (currentToken->type == writesym) {
        // Write statement
        get_next_token();
        expression();
        emit(SYS, 0, 2); // WRITE
    }
    // Empty statement is allowed
}

void condition() {
    if (currentToken->type == oddsym) {
        get_next_token();
        expression();
        emit(OPR, 0, 11); // ODD operation (mod 2)
    }
    else {
        expression();
        if (currentToken->type == eqlsym || currentToken->type == neqsym || 
            currentToken->type == lessym || currentToken->type == leqsym || 
            currentToken->type == gtrsym || currentToken->type == geqsym) {
            
            int relop = currentToken->type;
            get_next_token();
            expression();
            
            switch (relop) {
                case eqlsym: emit(OPR, 0, 8); break;  // EQL
                case neqsym: emit(OPR, 0, 9); break;   // NEQ
                case lessym: emit(OPR, 0, 10); break;  // LSS
                case leqsym: emit(OPR, 0, 11); break;  // LEQ
                case gtrsym: emit(OPR, 0, 12); break;  // GTR
                case geqsym: emit(OPR, 0, 13); break;  // GEQ
            }
        }
        else {
            error(12); // condition must contain comparison operator
        }
    }
}

void expression() {
    if (currentToken->type == plussym || currentToken->type == minussym) {
        int addop = currentToken->type;
        get_next_token();
        term();
        
        if (addop == minussym) {
            emit(OPR, 0, 1); // NEG
        }
    }
    else {
        term();
    }
    
    while (currentToken->type == plussym || currentToken->type == minussym) {
        int addop = currentToken->type;
        get_next_token();
        term();
        
        if (addop == plussym) {
            emit(OPR, 0, 2); // ADD
        }
        else {
            emit(OPR, 0, 3); // SUB
        }
    }
}

void term() {
    factor();
    while (currentToken->type == multsym || currentToken->type == slashsym) {
        int mulop = currentToken->type;
        get_next_token();
        factor();
        
        if (mulop == multsym) {
            emit(OPR, 0, 4); // MUL
        }
        else {
            emit(OPR, 0, 5); // DIV
        }
    }
}

void factor() {
    if (currentToken->type == identsym) {
        int sym_idx = find_symbol(currentToken->lexeme);
        if (sym_idx == -1) {
            error(6); // undeclared identifier
        }
        
        if (symbol_table[sym_idx].kind == 1) {
            emit(LIT, 0, symbol_table[sym_idx].val); // Constant
        }
        else {
            emit(LOD, 0, symbol_table[sym_idx].addr); // Variable
        }
        
        get_next_token();
    }
    else if (currentToken->type == numbersym) {
        emit(LIT, 0, atoi(currentToken->lexeme));
        get_next_token();
    }
    else if (currentToken->type == lparentsym) {
        get_next_token();
        expression();
        if (currentToken->type != rparentsym) {
            error(13); // right parenthesis must follow left parenthesis
        }
        get_next_token();
    }
    else if (currentToken->type == rparentsym) {
        error(16); // unmatched right parenthesis
    }
    else {
        error(14); // invalid factor
    }
}

void print_errors() {
    if (errorCount > 0) {
        printf("\nErrors:\n");
        for (int i = 0; i < errorCount; i++) {
            printf("Line %d, Column %d: %s\n", 
                   errors[i].line, errors[i].column, errors[i].message);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }    

    char* InputFile = argv[1];
    FILE *input = fopen(InputFile, "r");
    if (!input) {
        perror("Error opening file");
        return 1;
    }

    // First pass - lexical analysis
    printSourceProgram(input);
    scanTokens(input);
    
    print_errors();
    if (hasError) {
        fclose(input);
        return 1;
    }
    
    // Second pass - parsing and code generation
    program();
    
    // Print generated code
    printf("\nAssembly Code:\n");
    printf("Line OP L M\n");
    for (int i = 0; i < cx; i++) {
        printf("%4d %3d %d %d\n", i, code[i].op, code[i].L, code[i].M);
    }
    
    // Print symbol table
    printf("\nSymbol Table:\n");
    printf("Kind | Name | Value | Level | Address | Mark\n");
    printf("--------------------------------------------\n");
    for (int i = 0; i < sym_table_size; i++) {
        printf("%4d | %4s | %5d | %5d | %7d | %4d\n",
               symbol_table[i].kind,
               symbol_table[i].name,
               symbol_table[i].val,
               symbol_table[i].level,
               symbol_table[i].addr,
               symbol_table[i].mark);
    }
    
    fclose(input);
    return 0;
}
