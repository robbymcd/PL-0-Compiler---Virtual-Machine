#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int OP;
    int L;
    int M;
} Instruction;

int pas[512];
Instruction ir;
int bp = 0;
int sp = 0;
int pc = 0;
int halt = 1;

int base(int BP, int L) {
    int arb = BP; //arb = activation record base
    while (L > 0) {
        arb = pas[arb];
        L--;
    }
    return arb;
}

void initializePas() {
    for (int i = 0; i < 512; i++) {
        pas[i] = 0;
    }
}

int loadInstructions(const char* filename) {
    FILE *file = fopen(filename, "r");
    int IC = 0;
    int OP, L, M;
    while (fscanf(file, "%d %d %d", &OP, &L, &M) != EOF) {
        pas[IC] = OP;
        pas[IC+1] = L;
        pas[IC+2] = M;
        IC+=3;
    }
    fclose(file);
    return IC;
}

int main (int argc, char *argv[]) {
    initializePas();

    int numInstructions = loadInstructions(argv[1]);

    bp = numInstructions;
    sp = bp - 1;
    pc = 0;

    printf("\t\t\tPC\tBP\tSP\tstack\n");
    printf("Initial values:\t\t%d\t%d\t%d\n\n", pc, bp, sp);

    while (halt != 0) {
        // fetch
        ir.OP = pas[pc];
        ir.L = pas[pc + 1];
        ir.M = pas[pc + 2];
        pc = pc + 3;

        // execute
        int arb = base(bp, ir.L);
        switch(ir.OP) {
            case 1: // LIT
                sp++;
                pas[sp] = ir.M;
                //text output
                printf("\tLIT %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 2: // OPR
                switch(ir.M) {
                    case 0: // RTN
                        sp = bp -1;
                        bp = pas[sp + 2];
                        pc = pas[sp + 3];
                        // text output
                        printf("\tRTN %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 1: // ADD
                        pas[sp - 1] = pas[sp - 1] + pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tADD %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 2: // SUB
                        pas[sp - 1] = pas[sp - 1] - pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tSUB %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 3: // MUL
                        pas[sp - 1] = pas[sp - 1] * pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tMUL %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 4: // DIV
                        pas[sp - 1] = pas[sp - 1] / pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tDIV %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 5: // EQL
                        pas[sp - 1] = pas[sp - 1] == pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tEQL %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 6: // NEQ
                        pas[sp - 1] = pas[sp - 1] != pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tNEQ %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 7: // LSS
                        pas[sp - 1] = pas[sp - 1] < pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tLSS %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 8: // LEQ
                        pas[sp - 1] = pas[sp - 1] <= pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tLEQ %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 9: // GTR
                        pas[sp - 1] = pas[sp - 1] > pas[sp];
                        sp = sp - 1;
                        // text output
                        printf("\tGTR %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                    case 10: // GEQ
                        pas[sp - 1] = pas[sp - 1] >= pas[sp];
                        sp = sp - 1;
                        // text output 
                        printf("\tGEQ %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                }
                break;
            
            case 3: // LOD
                sp = sp + 1;
                pas[sp] = pas[base(bp, ir.L) + ir.M];
                // text output
                printf("\tLOD %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 4: // STO
                pas[base(bp, ir.L) + ir.M] = pas[sp];
                sp = sp - 1;
                // text output
                printf("\tSTO %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 5: // CAL
                pas[sp + 1] = base(bp, ir.L);
                pas[sp + 2] = bp;
                pas[sp + 3] = pc;
                bp = sp + 1;
                pc = ir.M;
                // text output
                printf("\tCAL %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 6: // INC
                sp = sp + ir.M;
                // text output
                printf("\tINC %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 7: // JMP
                pc = ir.M;
                // text output
                printf("\tJMP %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 8: // JPC
                if (pas[sp] == 0) {
                    pc = ir.M;
                }
                sp = sp - 1;
                // text output
                printf("\tJPC %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                break;

            case 9: // SYS
                switch(ir.M) {
                    case 1: // write
                        printf("Output result is: %d\n", pas[sp]);
                        sp = sp - 1;
                        // text output
                        printf("\tSYS %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;

                    case 2: // read
                        sp = sp + 1;
                        printf("Please Enter an Integer: ");
                        scanf("%d", &pas[sp]);
                        // text output
                        printf("\tSYS %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;

                    case 3: // halt
                        halt = 0;
                        // text output
                        printf("\tSYS %d\t%d\t%d\t%d\t%d\t", ir.L, ir.M, pc, bp, sp);
                        break;
                }
                break;
        }
        // print stack
        char currentAR[256] = "";
        char tmp[16];
        // store current AR
        for (int j = bp; j <= sp; j++) {
            sprintf(tmp, "%d ", pas[j]);
            strcat(currentAR, tmp);
        }

        int arPointer = pas[bp + 1]; 
        int previousSP = bp - 1;  
        // print ARs
        while (arPointer > 0) {
            for (int j = arPointer; j <= previousSP; j++) {
                printf("%d ", pas[j]);
            }

            printf("| ");  

            previousSP = arPointer - 1;  
            arPointer = pas[arPointer + 1];  
        }
        // print current AR
        printf("%s\n", currentAR);
    }

    return 0;
}