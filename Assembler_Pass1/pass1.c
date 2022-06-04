#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3-asm_pass1_u.c"

int main(int argc, char *argv[]){
	int		i, c, line_count;
	char	buf[LEN_SYMBOL];
	LINE	line;
	int		symbolCount = 0;
	int		symbolLoc[1000];
	char	symbolTable[1000][50];
	int		locCount = 0x0, startLoc = 0x0;
	
	if(argc < 2)
		printf("Usage: %s fname.asm\n", argv[0]);
	else{
		if(ASM_open(argv[1]) == NULL)
			printf("File not found!!\n");
		else{
			for(line_count = 1 ;(c = process_line(&line)) != LINE_EOF; line_count++){
				if(c == LINE_ERROR)
					printf("%03d : Error\n", line_count);
				else if(c == LINE_COMMENT)
					printf("%03d : Comment line\n", line_count);
				else{
					if(line_count = 1 && strcmp(line.op,"START") == 0){		//savre#[operand] as start address
                        startLoc = strtol(line.operand1,NULL,16);
                        locCount += startLoc;
                        printf("%06X : %12s %12s %12s,%12s (FMT=%X, ADDR=%X)\n", locCount, line.symbol, line.op, line.operand1, line.operand2, line.fmt, line.addressing);
                    }
					else{
						printf("%06X : %12s %12s %12s,%12s (FMT=%X, ADDR=%X)\n", locCount, line.symbol, line.op, line.operand1, line.operand2, line.fmt, line.addressing);
						if(line.symbol[0] != '\0'){		//Is symbol Judge
                            strcpy(symbolTable[symbolCount], line.symbol);
                            symbolLoc[symbolCount] = locCount;
                            symbolCount++;
                        }
						if(line.fmt == FMT0){		//Assembler Directive
							if(strcmp(line.op, "WORD") == 0)
								locCount += 3;
							else if(strcmp(line.op, "RESW") == 0)
								locCount += strtol(line.operand1,NULL,10)*3;							
							else if(strcmp(line.op, "BYTE") == 0){
								if(line.operand1[0] == 'C')
									locCount += (strlen(line.operand1)-3);
								else if(line.operand1[0] == 'X')
									locCount += (strlen(line.operand1)-3)/2;
							}
							else if(strcmp(line.op, "RESB") == 0)
								locCount += strtol(line.operand1,NULL,10);
						}
						else if(line.fmt == FMT1)
							locCount += 1;
						else if(line.fmt == FMT2)
							locCount += 2;
						else if(line.fmt == FMT3)
							locCount += 3;
						else if(line.fmt == FMT4)
							locCount += 4;
					}
				}
			}
			
			printf("\n");		
			printf("Program length = %X\n", locCount - startLoc);
			for(i = 0; i < symbolCount; i++)
				printf("%12s : %06X\n", symbolTable[i], symbolLoc[i]);
			ASM_close();
		}
	}
	
}
