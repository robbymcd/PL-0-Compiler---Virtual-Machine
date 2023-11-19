// Robert McDonald

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LEN 1000
#define MAX_NAME 11
#define MAX_NUM 5
#define MAX_SYMBOL_TABLE_SIZE 500
#define CODE_SIZE 512
#define MAX_LEVELS 5

// function prototypes
void lexicalAnalyzer(char* source, int *size, FILE* outputFile);
int isReservedWord(char* word);
int isSpecialSymbol(char* symbol);
int getNextToken();
void error(int i);
void program();
void block();
void constDeclaration();
int varDeclaration();
void statement();
void condition();
void expression();
void term();
void factor();
void emit(int op, int L, int M);
int symbolTableCheck(char* name, int checkCurrentScopeOnly);
void addToSymbolTable(int kind, char *name, int val, int level, int addr, int mark);
void printSymbolTable(FILE* outputFile);
int addName(char* name);
void procedure();
void exitScope(int level);

// token numbers
typedef enum {
    oddsym = 1, identsym, numbersym, plussym, minussym,
    multsym, slashsym, ifelsym, eqlsym, neqsym, lessym, leqsym,
    gtrsym, geqsym, lparentsym, rparentsym, commasym, semicolonsym,
    periodsym, becomessym, beginsym, endsym, ifsym, thensym,
    whilesym, dosym, callsym, constsym, varsym, procsym, writesym,
    readsym , elsesym
} token_type;

// token names
char* tokenStrings[] = {
    "", "oddsym", "identsym", "numbersym", "plussym", "minussym",
    "multsym", "slashsym", "ifelsym", "eqlsym", "neqsym", "lessym", "leqsym",
    "gtrsym", "geqsym", "lparentsym", "rparentsym", "commasym", "semicolonsym",
    "periodsym", "becomessym", "beginsym", "endsym", "ifsym", "thensym",
    "whilesym", "dosym", "callsym", "constsym", "varsym", "writesym",
    "readsym" , "elsesym", "procsym"
};

// token struct
typedef struct {
    char lexeme[MAX_NAME];
    int token;
    int index;
    int value;
} TokenPair;

// symbol table struct
typedef struct {
    int kind;
    char name[MAX_NAME];
    int val;
    int level;
    int addr;
    int mark;
} symbol;

// global symbol table
symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
// symbol table size counter
int symbolTableSize = 0;

// reserved words
TokenPair reservedWords[] = {
    {"const", constsym}, {"var", varsym},
    {"begin", beginsym}, {"end", endsym}, {"if", ifsym}, {"then", thensym},
    {"while", whilesym}, {"do", dosym}, {"read", readsym}, {"write", writesym}, {"odd", oddsym},
    {"call", callsym}, {"procedure", procsym}
};

// special symbols
TokenPair specialSymbols[] = {
    {"+", plussym}, {"-", minussym}, {"*", multsym}, {"/", slashsym}, {"(", lparentsym},
    {")", rparentsym}, {"=", eqlsym}, {",", commasym}, {".", periodsym}, {";", semicolonsym},
    {":=", becomessym}, {"<>", neqsym}, {"<=", leqsym}, {">=", geqsym}, {"<", lessym},
    {">", gtrsym}
};

// instruction struct
typedef struct {
    int OP;
    int L;
    int M;
} instruction;

// instruction array
instruction text[CODE_SIZE];
// code index
int cx = 0;

// token list
TokenPair *tokenList;
// name table
char *nameTable;
// name table count
int nameCount = 0;
// token index
int tx = 0;
// token
int token = 0;
// symbol table index
int symIdx = 0;
// lexigraphical level
int level = 0;

// check if word is a reserved word
int isReservedWord(char* word) {
    for (int i = 0; i < sizeof(reservedWords) / sizeof(TokenPair); i++) {
        if (strcmp(word, reservedWords[i].lexeme) == 0) {
            return reservedWords[i].token;
        }
    }
    return -1;
}

// check if symbol is a special symbol
int isSpecialSymbol(char* symbol) {
    for (int i = 0; i < sizeof(specialSymbols) / sizeof(TokenPair); i++) {
        if (strcmp(symbol, specialSymbols[i].lexeme) == 0) {
            return specialSymbols[i].token;
        }
    }
    return -1;
}

// emit
void emit(int op, int L, int M) {
    if (cx > CODE_SIZE) {
        printf("Exceeded maximum instruction count\n");
        exit(1);
    } else {
        text[cx].OP = op;
        text[cx].L = L;
        text[cx].M = M;
        cx++;
    }
}

// program
void program() {
    token = getNextToken();
    block();
    if (token != periodsym) {
        error(1);
        exit(1);
    }
    emit(9, level, 3); // SYS 3
}

void block() {
    int dx, tx0, cx0;
    dx = 3;
    tx0=tx-1;
    symbol_table[tx-1].addr=cx;
    emit(7, 0, 0); // JMP
    if (level > MAX_LEVELS) {
        printf("Error: Exceeded maximum nesting level\n");
        exit(1);
    }

    if (token == constsym) {
        constDeclaration();
    }

    if (token == varsym) {
        dx += varDeclaration();
    }

    if (token == procsym) {
        procedure();
    }

    text[symbol_table[tx0].addr].M = cx;
    emit(6, 0, dx); // INC
    statement();
    emit(2, 0, 0); // OPR
    exitScope(level);
}

void procedure() {
    while (token == procsym) {
        token = getNextToken();
        if (token != identsym) {
            error(2);
            exit(1);
        }
        if (symbolTableCheck(&nameTable[tokenList[tx-1].index * MAX_NAME], 1) != -1) {
            error(3);
            exit(1);
        }

        char identName[MAX_NAME] = "";
        strcpy(identName, &nameTable[tokenList[tx-1].index * MAX_NAME]);

        addToSymbolTable(3, identName, 0, level, 0, 0);

        token = getNextToken();

        if (token != semicolonsym) {
            error(6);
            exit(1);
        }

        level++;
        token = getNextToken();
        block();
        level--;

        if (token != semicolonsym) {
            error(6);
            exit(1);
        }

        token = getNextToken();
        
    }
}

// const declaration
void constDeclaration() {
    if (token == constsym) {
        do {
            token = getNextToken();
            if (token != identsym) {
                error(2);
                exit(1);
            }
            if (symbolTableCheck(&nameTable[tokenList[tx-1].index * MAX_NAME], 1) != -1) {
                error(3);
                exit(1);
            }
            // save identifier name
            char identName[MAX_NAME] = "";
            strcpy(identName, &nameTable[tokenList[tx-1].index * MAX_NAME]);
            
            token = getNextToken();
            if (token != eqlsym) {
                error(4);
                exit(1);
            }

            token = getNextToken();
            if (token != numbersym) {
                error(5);
                exit(1);
            }

            addToSymbolTable(1, identName, tokenList[tx-1].value, level, 0, 0);
            token = getNextToken();
    
        } while (token == commasym);

        if (token != semicolonsym) {
            error(6);
            exit(1);
        }
        token = getNextToken();
    }
}

int varDeclaration() {
    int numVars = 0;
    if (token == varsym) {
        do {
            numVars++;
            token = getNextToken();
            if (token != identsym) {
                error(2);
                exit(1);
            }
            int nameIndex = tokenList[tx-1].index;

            if (symbolTableCheck(&nameTable[nameIndex * MAX_NAME], 1) != -1) {
                printSymbolTable(stdout);
                error(3);
                exit(1);
            }
            addToSymbolTable(2, &nameTable[nameIndex * MAX_NAME], 0, level, 2+numVars, 0);
            token = getNextToken();
        } while (token == commasym);

        if (token != semicolonsym) {
            error(6);
            exit(1);
        }
        token = getNextToken();
    }
    return numVars;
}

void statement() {
    if (token == identsym) {
        int symIdx = symbolTableCheck(&nameTable[tokenList[tx-1].index * MAX_NAME], 0);

        if (symIdx == -1) {
            error(7);
            exit(1);
        }
        if (symbol_table[symIdx].kind != 2) {
            error(8);
            exit(1);
        }
        token = getNextToken();
        if (token != becomessym) {
            error(9);
            exit(1);
        }
        token = getNextToken();
        expression();
        emit(4, level, symbol_table[symIdx].addr); // STO
        return;
    }
    if (token == callsym) {
        token = getNextToken();
        if (token != identsym) {
            error(2);
            exit(1);
        } else {
            int symIdx = symbolTableCheck(&nameTable[tokenList[tx-1].index * MAX_NAME], 0);
            if (symIdx == -1) {
                error(7);
                exit(1);
            } else {
                if (symbol_table[symIdx].kind == 3) {
                    emit(5, level, symbol_table[symIdx].addr); // CAL
                } else {
                    error(8);
                    exit(1);
                }
            }
        }
        token = getNextToken();
    }

    if (token == beginsym) {
        do {
            token = getNextToken();
            statement();
        } while (token == semicolonsym);
        if (token != endsym) {
            error(10);
            exit(1);
        }
        token = getNextToken();
        return;
    }
    if (token == ifsym) {
        token = getNextToken();
        condition();
        int jpcIdx = cx;
        emit(8, level, 0); // JPC
        if (token != thensym) {
            error(11);
            exit(1);
        }
        token = getNextToken();
        statement();
        text[jpcIdx].M = cx*3;
        return;
    }
    if (token == whilesym) {
        token = getNextToken();
        int loopIdx = cx;
        condition();
        if (token != dosym) {
            error(12);
            exit(1);
        }
        token = getNextToken();
        int jpcIdx = cx;
        emit(8, level, 0); // JPC
        statement();
        emit(7, level, loopIdx); // JMP
        text[jpcIdx].M = cx*3;
        return;
    }
    if (token == readsym) {
        token = getNextToken();
        if (token != identsym) {
            error(2);
            exit(1);
        }
        int symIdx = symbolTableCheck(&nameTable[tokenList[tx-1].index * MAX_NAME], 0);
        if (symIdx == -1) {
            error(7);
            exit(1);
        }
        if (symbol_table[symIdx].kind != 2) {
            error(8);
            exit(1);
        }
        token = getNextToken();
        emit(9, level, 2); // SYS 2 (Read)
        emit(4, level, symbol_table[symIdx].addr); // STO
        return;
    }
    if (token == writesym) {
        token = getNextToken();
        expression();
        emit(9, level, 1); // SYS 1 (Write)
        return;
    }
}

// condition
void condition() {
    if (token == oddsym) {
        token = getNextToken();
        expression();
        emit(2, level, 11); // ODD
    } else {
        expression();
        if (token == eqlsym) {
            token = getNextToken();
            expression();
            emit(2, level, 5); // EQL
        } else if (token == neqsym) {
            token = getNextToken();
            expression();
            emit(2, level, 6); // NEQ
        } else if (token == lessym) {
            token = getNextToken();
            expression();
            emit(2, level, 7); // LSS
        } else if (token == leqsym) {
            token = getNextToken();
            expression();
            emit(2, level, 8); // LEQ
        } else if (token == gtrsym) {
            token = getNextToken();
            expression();
            emit(2, level, 9); // GTR
        } else {
            error(13);
            exit(1);
        }
    }
}

// expression
void expression() {
    term();
    while (token == plussym || token == minussym) {
        if (token == plussym) {
            token = getNextToken();
            term();
            emit(2, level, 1); // ADD
        } else if (token == minussym) {
            token = getNextToken();
            term();
            emit(2, level, 2); // SUB
        }
    }
}

// term
void term() {
    factor();
    while (token == multsym || token == slashsym) {
        if (token == multsym) {
            token = getNextToken();
            factor();
            emit(2, level, 3); // MUL
        } else if (token == slashsym) {
            token = getNextToken();
            factor();
            emit(2, level, 4); // DIV
        }
    }
}

// factor
void factor() {
    if (token == identsym) {
        int symIdx = symbolTableCheck(&nameTable[tokenList[tx-1].index * MAX_NAME], 0);
        if (symIdx == -1) {
            error(7);
            exit(1);
        }
        if (symbol_table[symIdx].kind == 1) { // const
            emit(1, level, symbol_table[symIdx].val); // LIT
        } else { // var
            emit(3, level, symbol_table[symIdx].addr); // LOD
        }
        token = getNextToken();
    } else if (token == numbersym) {
        emit(1, level, tokenList[tx-1].value); // LIT
        token = getNextToken();
    } else if (token == lparentsym) {
        token = getNextToken();
        expression();
        if (token != rparentsym) {
            error(14);
            exit(1);
        }
        token = getNextToken();
    } else {
        error(15);
        exit(1);
    }
}


// get next token
int getNextToken() {
    return tokenList[tx++].token;
}

// symbol table check
int symbolTableCheck(char* name, int checkCurrentScopeOnly) {
    for (int i = symbolTableSize; i >= 0; i--) {
        if (strcmp(name, symbol_table[i].name) == 0 && symbol_table[i].mark == 0) {
            if (checkCurrentScopeOnly && symbol_table[i].level != level) {
                continue;
            }
            return i;
        }
    }
    return -1;
}

void exitScope(int level) {
    for (int i = 0; i < symbolTableSize; i++) {
        if (symbol_table[i].level == level) {
            symbol_table[i].mark = 1;
        }
    }
}

void addToSymbolTable(int kind, char *name, int val, int level, int addr, int mark) {
    if (symbolTableSize >= MAX_SYMBOL_TABLE_SIZE) {
        printf("Error: Symbol table full\n");
    }
    symbol s;
    s.kind = kind;
    strncpy(s.name, name, MAX_NAME);
    s.val = val;
    s.level = level;
    s.addr = addr;
    s.mark = mark;

    symbol_table[symbolTableSize++] = s;
}

// lexical analyzer
void lexicalAnalyzer(char* source, int *size, FILE* outputFile) {
    // initialize variables
    int i = 0, j, k = 0;
    char word[MAX_NAME], symbol[3];
    int inComment = 0;
    
    // loop through source array
    while (i < strlen(source)) {
        // check for comments
        if (source[i] == '/' && source[i+1] == '*') {
            inComment = 1;
            i += 2;
            continue; 
        } else if (source[i] == '*' && source[i+1] == '/' && inComment == 1) {
            inComment = 0;
            i += 2;
            continue; 
        }

        if (inComment) {
            i++;
            continue;
        }

        // check for whitespace 
        if (isspace(source[i])) {
            i++;
            continue;
        }

        // check letter
        if (isalpha(source[i])) {
            j = 0;
            int errorPrinted = 0;
            // scan in word
            while (i < strlen(source) && (isalpha(source[i])) || isdigit(source[i])) {
                if (j < MAX_NAME-1) {
                    word[j++] = source[i++];
                } else { // if too long
                    printf("%s...\t", word);
                    printf("Error: Name too long.\n");
                    fprintf(outputFile, "%s...\t\t", word);
                    fprintf(outputFile, "Error: Name too long.\n");
                    exit(1);
                    errorPrinted = 1;
                    // skip to next whitespace
                    while (i < strlen(source) && isalpha(source[i] || isdigit(source[i]))) {
                        i++;
                    }
                    break;
                }
            }
            word[j] = '\0'; // null terminator

            int token = isReservedWord(word); 
            if (!errorPrinted) {
                if (token != -1) { // if reserved word
                    tokenList[k].token = token;
                    k++;
                } else { // if identifier
                    tokenList[k].token = identsym;
                    int index = addName(word);
                    tokenList[k].index = index;
                    k++;
                }
            }
        } else if (isdigit(source[i])) { // check number
            j = 0;
            int errorPrinted = 0;
            // scan in number
            while (isdigit(source[i])) {
                if (j < MAX_NUM) {
                    word[j++] = source[i++];
                } else { // if too long
                    printf("%s...\t", word);
                    printf("Error: Number too long.\n");
                    fprintf(outputFile, "%s...\t\t", word);
                    fprintf(outputFile, "Error: Number too long.\n");
                    exit(1);
                    errorPrinted = 1;
                    // skip to next whitespace
                    while (i < strlen(source) && isdigit(source[i])) {
                        i++;
                    }
                    break;
                }
            }
            word[j] = '\0'; // null terminator

            if (!errorPrinted) {
                tokenList[k].token = numbersym;
                tokenList[k].value = atoi(word);                
                k++;
            }
        } else { // else check special symbol
            symbol[0] = source[i];
            // check for special symbol w 2 characters
            if (i+1 < strlen(source)) {
                symbol[1] = source[i+1];
            } else { // if only 1 character
                symbol[1] = '\0';
            }
            symbol[2] = '\0'; // null terminator

            // check if special symbol
            int token = isSpecialSymbol(symbol);
            if (token != -1) { // if special symbol
                tokenList[k++].token = token;
                i += 2;
            } else { // if not special symbol
                symbol[1] = '\0';
                token = isSpecialSymbol(symbol);
                if (token != -1) {
                    tokenList[k++].token = token;
                    i++;
                } else { // if non supported symbol
                    printf("%s\t\t", symbol);
                    printf("Error: Invalid symbol.\n");
                    fprintf(outputFile, "%s\t\t\t", symbol);
                    fprintf(outputFile, "Error: Invalid symbol.\n");
                    exit(1);
                    i++;
                }
            } 
        }
        *size = k;
    }
}

void error(int i) {
    printf("Error: ");
    switch (i) {
        case 1:
            printf("program must end with period\n");
            break;
        case 2:
            printf("const, var, procedure, call must be followed by identifier\n");
            break;
        case 3:
            printf("symbol name has already been declared\n");
            break;
        case 4:
            printf("constants must be assigned with =\n");
            break;
        case 5:
            printf("constants must be assigned an integer value\n");
            break;
        case 6:
            printf("constant and variable declarations must be followed by a semicolon\n");
            break;
        case 7:
            printf("undeclared identifier\n");
            break;
        case 8:
            printf("only variable values may be altered\n");
            break;
        case 9:
            printf("assignment statements must use :=\n");
            break;
        case 10:
            printf("begin must be followed by end\n");
            break;
        case 11:
            printf("if must be followed by then\n");
            break;
        case 12:
            printf("while must be followed by do\n");
            break;
        case 13:
            printf("condition must contain comparison operator\n");
            break;
        case 14:
            printf("right parenthesis must follow left parenthesis\n");
            break;
        case 15:
            printf("arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
            break;
    }
}

void printSymbolTable(FILE* outputFile) {
    printf("%-4s | %-20s | %-6s | %-5s | %-7s | %-4s\n", "Kind", "Name", "Value", "Level", "Address", "Mark");
    printf("-------------------------------------------------------------------------------\n");
    //fprintf(outputFile, "%-4s | %-20s | %-6s | %-5s | %-7s | %-4s\n", "Kind", "Name", "Value", "Level", "Address", "Mark");
    //fprintf(outputFile, "-------------------------------------------------------------------------------\n");
    for (int i = 0; i < symbolTableSize; i++) {
        printf("%-4d | %-20s | %-6d | %-5d | %-7d | %-4d\n",
               symbol_table[i].kind,
               symbol_table[i].name,
               symbol_table[i].val,
               symbol_table[i].level,
               symbol_table[i].addr,
               symbol_table[i].mark);
        /*fprintf(outputFile, "%-4d | %-20s | %-6d | %-5d | %-7d | %-4d\n",
                symbol_table[i].kind,
                symbol_table[i].name,
                symbol_table[i].val,
                symbol_table[i].level,
                symbol_table[i].addr,
                symbol_table[i].mark);*/
    }
}

int addName(char* name) {
    for (int i = 0; i < nameCount; i++) {
        if (strcmp(&nameTable[i * MAX_NAME], name) == 0) {
            return i;
        }
    }
    strcpy(&nameTable[nameCount * MAX_NAME], name);
    nameCount++;
    return nameCount-1;
}


int main(int argc, char** argv) {
    // allocate memory
    char *source = malloc(MAX_LEN * sizeof(char));
    tokenList = malloc(MAX_LEN * sizeof(TokenPair));
    nameTable = malloc(MAX_LEN * MAX_NAME * sizeof(char));
    int size = 0;
    FILE* fp = fopen(argv[1], "r"); // open file
    FILE* outputFile = fopen("elf.txt", "w"); // open output file

    // error handling file opening
    if (fp == NULL) {
        printf("Error: File not found\n");
        free(source);
        free(tokenList);
        free(nameTable);
        return 1;
    }
    // initialize variables
    int i = 0;
    char c;
    int bufferSize = MAX_LEN;
    // scan file into source array
    while ((c = fgetc(fp)) != EOF) {
        if (i >= bufferSize - 1) {
            bufferSize *= 2;
            source = realloc(source, bufferSize);
        }
        source[i++] = c;
    }
    source[i] = '\0'; // null terminater
    fclose(fp); // close file

    // run lexical analyzer
    lexicalAnalyzer(source, &size, outputFile);

    // print input program
    printf("%s\n", source);
    printf("\n");

    token = tokenList[0].token;
    program();

    printf("No errors, program is syntactically correct.\n");

    // print assembly code
    printf("\nLine\tOP\tL\tM\n");
    for (int i = 0; i < cx; i++) {
        char op[4];
        if (text[i].OP == 1) {
            strcpy(op, "LIT");
        } else if (text[i].OP == 2) {
            strcpy(op, "OPR");
        } else if (text[i].OP == 3) {
            strcpy(op, "LOD");
        } else if (text[i].OP == 4) {
            strcpy(op, "STO");
        } else if (text[i].OP == 5) {
            strcpy(op, "CAL");
        } else if (text[i].OP == 6) {
            strcpy(op, "INC");
        } else if (text[i].OP == 7) {
            strcpy(op, "JMP");
        } else if (text[i].OP == 8) {
            strcpy(op, "JPC");
        } else if (text[i].OP == 9) {
            strcpy(op, "SYS");
        }
        printf("%d\t%s\t%d\t%d\n", i, op, text[i].L, text[i].M);
        fprintf(outputFile, "%d %d %d\n", text[i].OP, text[i].L, text[i].M);
    }


    fclose(outputFile); // close output file

    // free memory
    free(source);
    free(tokenList);
    free(nameTable);
    return 0;
}
               
