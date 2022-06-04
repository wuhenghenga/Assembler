#include <string.h>
#include <stdlib.h>
#include "optable.c"
 
/* Public variables and functions */
#define	ADDR_SIMPLE			0x01
#define	ADDR_IMMEDIATE		0x02
#define	ADDR_INDIRECT		0x04
#define	ADDR_INDEX			0x08

#define	LINE_EOF			(-1)
#define	LINE_COMMENT		(-2)
#define	LINE_ERROR			(0)
#define	LINE_CORRECT		(1)
#define	IMR_SIZE			100
#define	SYMTAB_SIZE			100
#define	RLD_SIZE			100

typedef struct InputLine{
	char		symbol[LEN_SYMBOL];
	char		op[LEN_SYMBOL];
	char		operand1[LEN_SYMBOL];
	char		operand2[LEN_SYMBOL];
	unsigned	code;
	unsigned	fmt;
	unsigned	addressing;	
} LINE;

typedef struct SymbolTable{
	char label[LEN_SYMBOL];
	int address;
} SYM_table;

typedef struct RegisterTable{
	char name[LEN_SYMBOL];
	int id;
} registerTable;

typedef struct RelocationDict{
	int loc;
	int halfbyte;
} RLD;

typedef struct IntermediateRecord {
	char 				symbol[LEN_SYMBOL];
	char				op[LEN_SYMBOL];
	char 				operand1[LEN_SYMBOL];
	char 				operand2[LEN_SYMBOL];
	unsigned long long int ObjectCode;
	unsigned short int	Loc;	//  The location of the instruction in memory
	unsigned			code;
	unsigned			addressing;
	unsigned			fmt;
}IntermediateRec;

int process_line(LINE *line);			/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT and Instruction information in *line*/
SYM_table SYMTAB[SYMTAB_SIZE];			// Symbol table variable
IntermediateRec* IMRArray[IMR_SIZE];	// Intermediate file variable
RLD RLDArray[RLD_SIZE];					// Relocation Dist store inst need to be relocated
int start_address;						// Program start address
int program_length;						// Total length of the program

int LOCCTR; //Location Counter
int Loc;
int Index = 0;	//index for IMR counter
int SymtabCnt = 0;
int SymIdx = 0;
int RegIdx = 0;
int RlcIdx = 0;
char End_operand[20];	// Stores the operand of the END indicator
int i,j;

unsigned short int FoundOnSymtab_flag = 0;
unsigned short int FoundOnOptab_flag = 0;
unsigned short int FoundOnRegtab_flag = 0;
unsigned short int START_flag = 0;	//check START flag
unsigned short int SIC_flag = 1;

registerTable REGTAB[] = {
	{ "A", 0 },
	{ "X", 1 },
	{ "L", 2 },
	{ "B", 3 },
	{ "S", 4 },
	{ "T", 5 },
	{ "F", 6 },
};

void RecordSymtab(char* symbol) {
	strcpy(SYMTAB[SymtabCnt].label, symbol);
	SYMTAB[SymtabCnt].address = LOCCTR;
	SymtabCnt++;
}

void RecordRelocation(int location){
	RLDArray[RlcIdx].loc = location + 1;	// save relocation starting position (+1 to avoid opcode)
	RLDArray[RlcIdx].halfbyte = 5;	//	position that need to relocated
	RlcIdx++;
}

int SearchSymtab(char* symbol) {	// Finding a symbol in the symbol table
	FoundOnSymtab_flag = 0;

	for (i = 0; i <= SymtabCnt; i++) {	
		if (!strcmp(SYMTAB[i].label, symbol)) {
			FoundOnSymtab_flag = 1;
			SymIdx = i;
			return (FoundOnSymtab_flag);
		}
	}
	return (FoundOnSymtab_flag);
}

int SearchRegtab(char* reg) {	// Finding a symbol in the symbol table
	FoundOnRegtab_flag = 0;
	int size = sizeof(REGTAB) / sizeof(registerTable);
	for (i = 0; i < size; i++) {	
		if (!strcmp(REGTAB[i].name, reg)) {
			FoundOnRegtab_flag = 1;
			RegIdx = i;
			return (FoundOnRegtab_flag);
		}
	}
	return (FoundOnRegtab_flag);
}

int isNum(char* str){	//check is num or not
	int i, len = strlen(str);
	for(i=0;i<len;i++){
		if(str[i] == '-'){
			continue;
		}else if(str[i] < '0' || str[i] > '9'){
			return 0;
		}
	}
	return 1;
}

int isFloatNum(char* str) {
	int i, len = strlen(str), f = 0;
	for (i = 0; i < len; ++i) {
		if ('0' > str[i] || '9' < str[i]) {
			if (str[i] == '.' && f == 0) {
				f = 1;
				continue;
			}
			if (str[i] == '-') continue;
			return 0;
		}
	}
	return (f != 0) ? 1 : 0;
}

unsigned long filterNumber(int diff, int hexbits){	// filtting negative number according to the bit
	if(diff >= 0){			// if positive, escape
		return diff;
	}
	if(hexbits == 5) {	// if 32 bits, clear bit after 20 bits from left
		diff ^= 0xFFF00000;
	}else{						// if 20 bits, clear bit after 12 bits from left
		diff ^= 0xFFFFF000;
	}
	return diff;
}

int StrToDec(char* c) {
	int dec_num = 0;
	char temp[10];
	strcpy(temp, c);

	int len = strlen(c);
	for (i = len - 1, j = 1; i >= 0; i--){
		if (temp[0] == '-') {
			continue;
		}
		dec_num = dec_num + (int)(temp[i] - '0') * j;
		j = j * 10;
	}
	return (temp[0] == '-') ? (-dec_num) : (dec_num);
}

int StrToHex(char* c){
	int hex_num = 0;
	char temp[10];
	strcpy(temp, c);
	
	int len = strlen(temp);
	for(i = len-1, j = 1; i >= 0; i--){
		if(temp[i] >= '0' && temp[i] <= '9')
			hex_num = hex_num + (int)(temp[i] - '0') * j;
		else if (temp[i] >= 'A' && temp[i] <= 'F')	//10~15(UpCast)
			hex_num = hex_num + (int)(temp[i] - 'A' + 10) * j;
		else if (temp[i] >= 'a' && temp[i] >= 'f')	//10~15(LwCast)
			hex_num = hex_num + (int)(temp[i] - 'a' + 10) * j;
		j = j * 16; // to left value
	}
	return (hex_num);
}

int ComputeLen(char* c) {	// Calculate length of ASCII code or hexadecimal number
	unsigned int b;
	char len[32];

	strcpy(len, c);
	if (len[0] == 'C' || len[0] == 'c' && len[1] == '\'') {	// Starting with C'
		for (b = 2; b <= strlen(len); b++) {
			if (len[b] == '\'') {
				b -= 2;	// When meeting last ('), b-2 exit loop
				break;
			}
		}
	}
	if (len[0] == 'X' || len[0] == 'x' && len[1] == '\'')	// Starting with X'
		b = 1;
	return (b);
}

/***** Private variable and function *****/
void init_LINE(LINE *line){
	line->symbol[0] = '\0';
	line->op[0] = '\0';
	line->operand1[0] = '\0';
	line->operand2[0] = '\0';
	line->code = 0x0;
	line->fmt = 0x0;
	line->addressing = ADDR_SIMPLE;
}

int process_line(LINE *line){
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT */
	char		buf[LEN_SYMBOL];
	int			c;
	int			state;
	int			ret;
	Instruction	*op;
	
	c = ASM_token(buf);		/* get the first token of a line */
	if(c == EOF)
		return LINE_EOF;
	else if((c == 1) && (buf[0] == '\n'))	/* blank line */
		return LINE_COMMENT;
	else if((c == 1) && (buf[0] == '.')){	/* a comment line */
		do{
			c = ASM_token(buf);
		} while((c != EOF) && (buf[0] != '\n'));
		
		return LINE_COMMENT;
	}
	else{
		init_LINE(line);
		ret = LINE_ERROR;
		state = 0;
		while(state < 8){
			switch(state){
				case 0:
				case 1:
				case 2:
					op = is_opcode(buf);
					if((state < 2) && (buf[0] == '+')){	/* + */
						line->fmt = FMT4;
						state = 2;
					}
					else if(op != NULL){	/* INSTRUCTION */	
						strcpy(line->op, op->op);
						line->code = op->code;
						state = 3;
						if(line->fmt != FMT4)
							line->fmt = op->fmt & (FMT1 | FMT2 | FMT3);
						else if((line->fmt == FMT4) && ((op->fmt & FMT4) == 0)){	/* INSTRUCTION */	
							printf("ERROR at token %s, %s cannot use format 4\n", buf, buf);
							ret = LINE_ERROR;
							state = 7;
						}
					}				
					else if(state == 0){	/* SYMBOL */
						strcpy(line->symbol, buf);
						state = 1;
					}
					else{		/* ERROR */
						printf("ERROR at token %s\n", buf);
						ret = LINE_ERROR;
						state = 7;		/* skip following tokens in the line */
					}
					break;	
				case 3:
					if(line->fmt == FMT1 || line->code == 0x4C){	/* no operand needed */
						if(c == EOF || buf[0] == '\n'){
							ret = LINE_CORRECT;
							state = 8;
						}
						else{		/* COMMENT */
							ret = LINE_CORRECT;
							state = 7;
						}
					}
					else{
						if(c == EOF || buf[0] == '\n'){
							ret = LINE_ERROR;
							state = 8;
						}
						else if(buf[0] == '@' || buf[0] == '#'){
							line->addressing = (buf[0] == '#') ? ADDR_IMMEDIATE : ADDR_INDIRECT;
							state = 4;
						}
						else{	/* get a symbol */
							op = is_opcode(buf);
							if(op != NULL){
								printf("Operand1 cannot be a reserved word\n");
								ret = LINE_ERROR;
								state = 7; 		/* skip following tokens in the line */
							}
							else{
								strcpy(line->operand1, buf);
								state = 5;
							}
						}
					}			
					break;		
				case 4:
					op = is_opcode(buf);
					if(op != NULL){
						printf("Operand1 cannot be a reserved word\n");
						ret = LINE_ERROR;
						state = 7;		/* skip following tokens in the line */
					}
					else{
						strcpy(line->operand1, buf);
						state = 5;
					}
					break;
				case 5:
					if(c == EOF || buf[0] == '\n'){
						ret = LINE_CORRECT;
						state = 8;
					}
					else if(buf[0] == ','){
						state = 6;
					}
					else	/* COMMENT */{
						ret = LINE_CORRECT;
						state = 7;		/* skip following tokens in the line */
					}
					break;
				case 6:
					if(c == EOF || buf[0] == '\n'){
						ret = LINE_ERROR;
						state = 8;
					}
					else{	/* get a symbol */
						op = is_opcode(buf);
						if(op != NULL){
							printf("Operand2 cannot be a reserved word\n");
							ret = LINE_ERROR;
							state = 7;		/* skip following tokens in the line */
						}
						else{
							if(line->fmt == FMT2){
								strcpy(line->operand2, buf);
								ret = LINE_CORRECT;
								state = 7;
							}
							else if((c == 1) && (buf[0] == 'x' || buf[0] == 'X')){
								strcpy(line->operand2, buf);
								line->addressing = line->addressing | ADDR_INDEX;
								ret = LINE_CORRECT;
								state = 7;		/* skip following tokens in the line */
							}
							else{
								printf("Operand2 exists only if format 2  is used\n");
								ret = LINE_ERROR;
								state = 7;		/* skip following tokens in the line */
							}
						}
					}
					break;
				case 7:	/* skip tokens until '\n' || EOF */
					if(c == EOF || buf[0] =='\n')
						state = 8;
					break;										
			}
			if(state < 8)
				c = ASM_token(buf);  /* get the next token */
		}	
		return ret;
	}
}

void freeAll(){
	for (i = 0; i <= Index; i++) {
		free(IMRArray[i]);
	}
}

void PrintIMRArray(){
	printf("\n");
	for(i=0;i<Index;i++){
		if(strcmp(IMRArray[i]->op, "END"))
			printf("%06X\t", IMRArray[i]->Loc);
		else
			printf("\t");
		if(IMRArray[i]->fmt == 8){
			printf("+");
		}else{
			printf(" ");
		}
		printf("%-6s\t", IMRArray[i]->op);
		if(IMRArray[i]->addressing == 2){
			printf("#");
		}else if(IMRArray[i]->addressing == 4){
			printf("@");
		}else{
			printf(" ");
		}
		printf("%s", IMRArray[i]->operand1);
		if(IMRArray[i]->operand2[0] != '\0'){
			printf(",%s     \t", IMRArray[i]->operand2);
		}else{
			printf("\t\t");
		}
		if(IMRArray[i]->fmt == 6){
			printf("%08X\n", IMRArray[i]->ObjectCode);
		}else if(IMRArray[i]->fmt == 4 || !strcmp(IMRArray[i]->op, "WORD")){
			printf("%06X\n", IMRArray[i]->ObjectCode);
		}else if(IMRArray[i]->fmt == 2){
			printf("%04X\n", IMRArray[i]->ObjectCode);
		}else if(!strcmp(IMRArray[i]->op, "BYTE")){
			printf("%02X\n", IMRArray[i]->ObjectCode);
		}else{
			printf("\n");
		}
			
	}
}

void CreateSymbolTable(){
	FILE *fptr_sym;
	fptr_sym = fopen("symtab.txt", "w");
	
	printf("\n### Symbol Table ###\n");
	printf("   LABLE   LOC\n");
	fprintf(fptr_sym, "   LABLE   LOC\n");
	for(i=0;i<SymtabCnt;i++){
		printf("%8s : %04X\n", SYMTAB[i].label, SYMTAB[i].address);
		fprintf(fptr_sym, "%8s : %04X\n", SYMTAB[i].label, SYMTAB[i].address);
	}
	fclose(fptr_sym);
}

void CreateObjectCode(){	// Object file creation
	int TRecordFirst_address;
	int TRecordLast_address;
	int first_index;
	int last_index;
	int tmp_address;
	int instnum;	// The maximum number of object codes can be displayed in a line
	int line_code[20];
	unsigned line_fmt[20];
	unsigned long long int line_objcode[20];	// to contain 6-byte object code for each Text Record
	char line_operand[20][7];
	
	
	FILE *fptr_objCode;
	fptr_objCode = fopen("sic.obj", "w");
	if(fptr_objCode == NULL){	//Exception if cannot write sic.obj
		printf("ERROR: Unable to write sic.obj\n");
		freeAll();exit(1);
	}
	
	printf("\nCreating ObjCode\n");
	int loop=0;
	
	// write Header record
	if(!strcmp(IMRArray[loop]->op, "START")){	//check OPCODE is "START"
		// write Header record to .obj
		printf("H%-6s%06X%06X\n", IMRArray[loop]->symbol, start_address, program_length);
		fprintf(fptr_objCode, "H%-6s%06X%06X\n", IMRArray[loop]->symbol, start_address, program_length);
		loop++;
	}
	// write Text record
	while(1){
		TRecordFirst_address = IMRArray[loop]->Loc;			// save TRecord start record form a line
		TRecordLast_address = TRecordFirst_address + 30;	// save TRecord last record form a line
		first_index = loop;	// Initialize the first index value of a loop
		
		// loop for each line
		for(instnum = 0, tmp_address = TRecordFirst_address; tmp_address <= TRecordLast_address; loop++){
			// tmp_address : line limit control
			
			if(!strcmp(IMRArray[loop]->op, "END")){
				break;	// if "END", break line loop
			}else if(strcmp(IMRArray[loop]->op, "RESB") 		// if is Operation, save for print object code
				&& strcmp(IMRArray[loop]->op, "RESW") 
				&& strcmp(IMRArray[loop]->op, "BASE")
				&& strcmp(IMRArray[loop]->op, "NOBASE")){
					line_fmt[instnum] = IMRArray[loop]->fmt;
					line_objcode[instnum] = IMRArray[loop]->ObjectCode;
					line_code[instnum] = IMRArray[loop]->code;
					strcpy(line_operand[instnum], IMRArray[loop]->operand1);
					last_index = loop + 1;	// save last loc of a line
					instnum++;	// Increase the number of instructions in a line by one
			}
			
			tmp_address = IMRArray[loop + 1]->Loc;
			//printf("tmp: %04X", tmp_address);
			// store next loc for after a loop, check is line limit or not
			tmp_address = IMRArray[loop + 1]->Loc;	//get current inst bytes used
			if(IMRArray[loop + 1]->fmt == 4){							// if next inst is 3-bytes, tmp_address + 3;
				tmp_address += 3;
			}else if(IMRArray[loop + 1]->fmt == 2){						// if next inst is 4-bytes, tmp_address + 4;
				tmp_address += 2;
			}else if(IMRArray[loop + 1]->fmt == 6){						// if next inst is 4-bytes, tmp_address + 4;
				tmp_address += 4;
			}else{
				if(!strcmp(IMRArray[loop + 1]->op, "WORD")
				|| !strcmp(IMRArray[loop + 1]->op, "BYTE")){
					if(isFloatNum(IMRArray[loop + 1]->operand1)){		// if next inst is float  , tmp_address + 6;
						tmp_address += 6;
					}else if(!strcmp(IMRArray[loop + 1]->op, "WORD")){	// if next inst is "WORD" , tmp_address + 3;
						tmp_address += 3;
					}else if(!strcmp(IMRArray[loop + 1]->op, "BYTE")){	// if next inst is "BYTE" , tmp_address + 1-3;
						tmp_address += ComputeLen(IMRArray[loop + 1]->operand1);
					}
				}
			}
			//printf(" next : %04X\n", tmp_address);
		}
		
		// Output Header of Text Record
		printf("T%06X%02X", TRecordFirst_address, (IMRArray[last_index]->Loc - TRecordFirst_address));
		fprintf(fptr_objCode, "T^%06X^%02X", TRecordFirst_address, (IMRArray[last_index]->Loc - TRecordFirst_address));
		
		// Output Content of Text Record
		for(i = 0;i < instnum;i++){	//loop content in a Text Record line
			if(line_code[i] == 0x101){					// is "BYTE"
				if(line_operand[i][0] == 'X' || line_operand[i][0] == 'x'){
					printf("%02X", line_objcode[i]);
					fprintf(fptr_objCode, "%02X", line_objcode[i]);
				}else if(isFloatNum(line_operand[i])){
					printf("%012X", line_objcode[i]);
					fprintf(fptr_objCode, "%012X", line_objcode[i]);
				}else{
					printf("%06X", line_objcode[i]);
					fprintf(fptr_objCode, "%06X", line_objcode[i]);
				}
			}else{
				if(line_code[i] < 0x101){
					//printf("Searc\n");
					if(line_fmt[i] == 6){				// fmt 4
						printf("%08X", line_objcode[i]);
						fprintf(fptr_objCode, "%08X", line_objcode[i]);
					}else if(line_fmt[i] == 4){			// fmt 3
						printf("%06X", line_objcode[i]);
						fprintf(fptr_objCode, "%06X", line_objcode[i]);
					}else if(line_fmt[i] == 2){			// fmt 2
						printf("%04X", line_objcode[i]);
						fprintf(fptr_objCode, "%04X", line_objcode[i]);
					}else if(line_fmt[i] == 1){			// fmt 1
						printf("%02X", line_objcode[i]);
						fprintf(fptr_objCode, "%02X", line_objcode[i]);
					}
				}else if(isFloatNum(line_operand[i])){	// is float num
					printf("%012X", line_objcode[i]);
					fprintf(fptr_objCode, "%012X", line_objcode[i]);
				}else{									// is "WORD"
					printf("%06X", line_objcode[i]);
					fprintf(fptr_objCode, "%06X", line_objcode[i]);
				}
			}
		}
		printf("\n");
		fprintf(fptr_objCode, "\n");
		
		if (!strcmp(IMRArray[loop]->op, "END")){	// Escape while statement when encountering END assembler directive
			break;
		}
		
	}
	// Memory Relocation Record
	for(i = 0;i < RlcIdx;i++){
		printf("M%06X%02X\n", RLDArray[i].loc, RLDArray[i].halfbyte);
		fprintf(fptr_objCode, "M%06X%02X\n", RLDArray[i].loc, RLDArray[i].halfbyte);
	}
	
	// END Record
	printf("E%06X\n", start_address);
	fprintf(fptr_objCode, "E%06X\n", start_address);
	
	fclose(fptr_objCode);
}

int main(int argc, char *argv[]){
	int		i, c, line_count;
	char	buf[LEN_SYMBOL];
	LINE	line;

	if(argc < 2){
		printf("Usage: %s fname.asm\n", argv[0]);
	}
	else{
		if(ASM_open(argv[1]) == NULL)
			printf("File not found!!\n");
		else{
			for(line_count = 1 ; (c = process_line(&line)) != LINE_EOF; line_count++){
				/* Doing Pass 1*/
				if(c != LINE_COMMENT && c != LINE_ERROR){
					IMRArray[Index] = (IntermediateRec*)malloc(sizeof(IntermediateRec));
					strcpy(IMRArray[Index]->symbol, line.symbol);
					strcpy(IMRArray[Index]->op, line.op);
					strcpy(IMRArray[Index]->operand1, line.operand1);
					strcpy(IMRArray[Index]->operand2, line.operand2);
					IMRArray[Index]->ObjectCode = 0;
					IMRArray[Index]->code = line.code;
					IMRArray[Index]->addressing = line.addressing;
					IMRArray[Index]->fmt = line.fmt;
					
					if(IMRArray[Index]->addressing != 1 && IMRArray[Index]->addressing != 9) SIC_flag = 0;	// is using SIC/XE
					
					Loc = 0;
					if(!START_flag){	// havnt found 'START'
						if(!strcmp(line.op, "START")){		// if opcode = START
							LOCCTR = StrToHex(line.operand1);	// init LOCCTR to START.add
							start_address = LOCCTR;				// record start address
							START_flag = 1;
						}
					}else{
						if(strcmp(line.symbol, "END")){	// if opcode != END
							// Symbol check
							if(line.symbol[0] != '\0'){		// have symbol
								if(SearchSymtab(line.symbol)){
									printf("ERROR: Duplicate Symbol\n");
									ASM_close();
									freeAll();
									return 0;
								}else{
									RecordSymtab(line.symbol);	// Record Symbol to Symtable
								}
							}
							// Opcode check
							if(line.fmt == 2){			// format is 2
								Loc += 2;
							}else if(line.fmt == 4){	// format is 3
								Loc += 3;
							}else if(line.fmt == 8){	// format is 4
								Loc += 4;
							}else if(line.fmt == 0){	// pseudo instruction
								if(!strcmp(line.op, "WORD")){		// Secure 3 bytes (1 WORD)
									if(isFloatNum(line.operand1)){
										Loc += 6;
									}else{
										Loc += 3;
									}
								}else if(!strcmp(line.op, "RESW")){	// Secure memory as WORD of num(operands)
									Loc += (3 * StrToDec(line.operand1));
								}else if(!strcmp(line.op, "RESB")){	// Secure memory as BYTE of num(operands)
									Loc += StrToDec(line.operand1);
								}else if(!strcmp(line.op, "BYTE")){	// Secure 1 byte
									if(isFloatNum(line.operand1)){
										Loc += 6;
									}else{
										Loc += ComputeLen(line.operand1);
									}
								}else if(!strcmp(line.op, "BASE") || !strcmp(line.op, "NOBASE")){	// if BASE/NOBASE
									//LOCTTR = LOCTTR;
								}
							}else{
								printf("ERROR: Invalid Operation Code[%s]\n", line.op);
								ASM_close();
								freeAll();
							}
						}else {	// Save operand when encountering END Assembler Directive
							strcpy(End_operand, line.operand1);
						}
					}
					IMRArray[Index]->Loc = LOCCTR;

					Index++;
					LOCCTR += Loc;
				}
			}
			program_length = LOCCTR - start_address;
			printf("\nProgram length : %04X\n", program_length);
			CreateSymbolTable();
			ASM_close();
		}
		
		/********************************** PASS 2 ***********************************/
		unsigned long inst_objCode;
		unsigned long inst_opcode;		// Current opcode 		"0x00XX0000 / 0xXX000000"
		unsigned long inst_addrmode;	// Immediate, Indirect Addressing Mode
		unsigned long inst_relative;	// relative addressing mode
		unsigned long inst_index;		// index Addressing Mode
		unsigned long inst_extended;	// Extended format 4 	"0x00000000 / 0x00100000"
		unsigned long inst_address;		// variable of current operand
		//int inst_byte;					// variable of current byte used
		int Imp_loop;			// imp loop counter
		int base_register = -1;	// flag base register addressing mode
		int diff;				// variable stores the displacement
		char oprand[2][20];		// Temporary regName holder for compare
		
		for(Imp_loop = 0; Imp_loop < Index; Imp_loop++){
			// Initializing each variable
			inst_objCode	= 0;
			inst_opcode 	= 0;
			inst_addrmode	= 0;
			inst_relative	= 0;
			inst_index		= 0;
			inst_extended	= 0;
			inst_address	= 0;
			oprand[0][0]	= '\0';
			oprand[1][0]	= '\0';
			
			inst_opcode = IMRArray[Imp_loop]->code;	// copy opcode
			
			if(inst_opcode < 0x100){	// is opcode
				
				if(SIC_flag == 1){	// if is SIC
					if(SearchSymtab(IMRArray[Imp_loop]->operand1)){	// if is symbol
						IMRArray[Imp_loop]->ObjectCode = (inst_opcode << 16) + SYMTAB[SymIdx].address;
					}
					if(IMRArray[Imp_loop]->addressing == 9){		// is using index addressing mode
						IMRArray[Imp_loop]->ObjectCode += 0x008000;
					}
					continue;
				}
				
				if(inst_opcode == 0x4C){	// if "RSUB", (Left Shift + Simple Addressing Mode) and Store
					IMRArray[Imp_loop]->ObjectCode = ((inst_opcode << 16) + 0x030000);
					continue;
				}

				// check extended format 4
				if(IMRArray[Imp_loop]->fmt == 8){				// if fmt 4, flag e = 1
					inst_extended = 0x00100000;						// store extended holder
					if(IMRArray[Imp_loop]->addressing == 1){			// Simple Addressing Mode
						inst_addrmode = 0x03000000;
					}else if(IMRArray[Imp_loop]->addressing == 2){		// Immediate Addressing Mode
						inst_addrmode = 0x01000000;
					}else if(IMRArray[Imp_loop]->addressing == 4){		// Indirect Addressing Mode
						inst_addrmode = 0x02000000;
					}
					IMRArray[Imp_loop]->fmt = 6;				// change fmt to op displacement
				}else if(IMRArray[Imp_loop]->fmt == 4 && SIC_flag == 0){			// not fmt 4
					// check addressing mode, and store addressing holder
					if(IMRArray[Imp_loop]->addressing == 1 || IMRArray[Imp_loop]->addressing == 9){	// Simple Addressing Mode
						inst_addrmode = 0x030000;
					}else if(IMRArray[Imp_loop]->addressing == 2){									// Immediate Addressing Mode
						inst_addrmode = 0x010000;
					}else if(IMRArray[Imp_loop]->addressing == 4){									// Indirect Addressing Mode
						inst_addrmode = 0x020000;
					}
				}
				// store opcode to opcode_holder
				inst_opcode <<= (IMRArray[Imp_loop]->fmt * 4);	// Shift left for each fmt type
				
				if(IMRArray[Imp_loop]->fmt >= 4){				// In Case fmt 3/4
					if(IMRArray[Imp_loop]->addressing == 9){	// Index Addressing Mode
						inst_index = 0x008000;
						inst_index <<= 4 * (IMRArray[Imp_loop]->fmt -4);
					}
					if(SearchSymtab(IMRArray[Imp_loop]->operand1)){	//found Symbol
						if(IMRArray[Imp_loop]->fmt > 4){				// fmt 4
							inst_address = SYMTAB[SymIdx].address;
							RecordRelocation(IMRArray[Imp_loop]->Loc);
						}else {											// relative Addressing mode	(disp)
							diff = SYMTAB[SymIdx].address - IMRArray[Imp_loop]->Loc - 3;
							if(diff >= -2048 && diff < 2048){				// PC relative
								inst_address = 0x002000;
								inst_address += filterNumber(diff, 3);			// if negative num, left only 3/5 hexbit from right
								
							}else{											// If PC relative fails, try Base relative
								diff = SYMTAB[SymIdx].address - base_register;
								if(base_register != -1 && diff >= 0 && diff < 4096){	
									inst_address = 0x004000;
									inst_address += diff;
								}else{	// Exception handling when pc & base relative addressing fails
									printf("Base register: %d Disp: %d\n", base_register, diff);
									printf("ERROR: CANNOT present relative addressing mode[line : %X]\n", IMRArray[Imp_loop]->Loc);
									freeAll();exit(1);
								}
							}
						}
						
					}else{	// When operand not in symbol table
						if(IMRArray[Imp_loop]->addressing == 2 && isNum(IMRArray[Imp_loop]->operand1)){	// is num and in immediate addressing mode
							inst_address = filterNumber(StrToDec(IMRArray[Imp_loop]->operand1), (IMRArray[Imp_loop]->fmt == 4) ? 5 : 3); // oprand1 to num, addressing with 0xXXXXX/0xXXX
						}else{	// Exception handling when operand1 not symbol or num
							printf("ERROR: Operand %s isn't exist [line : %X]\n", IMRArray[Imp_loop]->operand1, IMRArray[Imp_loop]->Loc);
							freeAll();
							exit(1);
						}
						
					}
				}else if(IMRArray[Imp_loop]->fmt == 2){	// In Case fmt 2
					strcpy (oprand[0], IMRArray[Imp_loop]->operand1);
					strcpy (oprand[1], IMRArray[Imp_loop]->operand2);
					
					for(j=0;j<2;j++){	// loop for oprand1 / oprand2
						if(SearchRegtab(oprand[j])){
							inst_address += REGTAB[RegIdx].id;
							if(j == 0){
								inst_address <<= 4;
							}
						}else if(!strcmp(IMRArray[Imp_loop]->op, "SVC") || !strcmp(IMRArray[Imp_loop]->op, "SHIFTL") || !strcmp(IMRArray[Imp_loop]->op, "SHIFTR")){
							if(isNum(oprand[j])){
								inst_address += StrToDec(oprand[j]);
							}
						}else if(oprand[j][0] != '\0'){
							printf("ERROR: Invalid Register\n");
							freeAll();exit(1);
						}
					}
				}
				
				IMRArray[Imp_loop]->ObjectCode = inst_opcode + inst_addrmode + inst_index + inst_relative + inst_extended + inst_address;
			}else if(!strcmp(IMRArray[Imp_loop]->op, "WORD")){	// if opcode is "WORD"
				
				if(isFloatNum(IMRArray[Imp_loop]->operand1)){									// if is float num
					//IMRArray[Imp_loop]->ObjectCode = convert48bitFloatNum(IMRArray[Imp_loop]->operand1);
				}else{
					IMRArray[Imp_loop]->ObjectCode = StrToDec(IMRArray[Imp_loop]->operand1);
				}
			}else if(!strcmp(IMRArray[Imp_loop]->op, "BYTE")){	// if opcode is "BYTE"
				strcpy (oprand[0], IMRArray[Imp_loop]->operand1);
				IMRArray[Imp_loop]->ObjectCode = 0;												//set to default
				
				if(isFloatNum(oprand[0])){														// if is float num
					//IMRArray[Imp_loop]->ObjectCode = convert48bitFloatNum(oprand[0]);
				}else{
					if(oprand[0][0] == 'C' || oprand[0][0] == 'c' && oprand[0][1] == '\''){	// if is a char
						for(j=2;j <= strlen(oprand[0]) - 2; j++){
							IMRArray[Imp_loop]->ObjectCode <<= 8;									// shift 2Hex
							IMRArray[Imp_loop]->ObjectCode += (int)oprand[0][j];
						}
					}
					if(oprand[0][0] == 'X' || oprand[0][0] == 'x' && oprand[0][1] == '\''){	// if is a Hex
						IMRArray[Imp_loop]->ObjectCode += StrToHex(oprand[0]);
						IMRArray[Imp_loop]->ObjectCode >>= 4;										// shift 1Hex
					}
				}
			}else if(!strcmp(IMRArray[Imp_loop]->op, "BASE")){	// if opcode is "BASE"
				IMRArray[Imp_loop]->ObjectCode = 0;					//set to default
				if(SearchSymtab(IMRArray[Imp_loop]->operand1)){
					base_register = SYMTAB[SymIdx].address;		// store base_register
				}else{
					printf("ERROR: No Symbol[%s] in SYMTAB [line : %X]\n", IMRArray[Imp_loop]->operand1, IMRArray[Imp_loop]->Loc);
					freeAll();exit(1);
				}
			}else if(!strcmp(IMRArray[Imp_loop]->op, "NOBASE")){// if opcode is "NOBASE"
				IMRArray[Imp_loop]->ObjectCode = 0;					//set to default
				base_register = -1;
			}
			printf("%s\t%s\t%s\t%s\t%X\n", IMRArray[Imp_loop]->symbol, IMRArray[Imp_loop]->op, IMRArray[Imp_loop]->operand1, IMRArray[Imp_loop]->operand2, IMRArray[Imp_loop]->ObjectCode);
		}
		
		//print
		PrintIMRArray();
		CreateObjectCode();
		freeAll();
	}
	
	return 0;
}
