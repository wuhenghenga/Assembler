#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct loadMap {
	char controlSection[10];
	int address;
	int length;
	char asymbolName[10];
	int aAddress;
	char bsymbolName[10];
	int bAddress;
} linkLoader;

int main(int argc, char **argv){
	linkLoader loader[3];
	loader[0].address = strtol(argv[1], NULL, 16);
	
	for (int i = 0; i < argc - 2; i++){
		int isBreak = 0;
		FILE *fileprog = fopen(argv[i + 2], "r");
		while(!feof(fileprog)){
			char objCode[100];
			fscanf(fileprog, "%s", objCode);
			if(objCode[0] == 'H'){
				strcpy(loader[i].controlSection, objCode + 1);
				fscanf(fileprog, "%s", objCode);
                loader[i].length = strtol(objCode, NULL, 16);
			}
			else if(objCode[0] == 'D'){
				if(i > 0)	loader[i].address = loader[i - 1].address + loader[i - 1].length;
				strcpy(loader[i].asymbolName, objCode + 1);
				fscanf(fileprog, "%s", objCode);
				strcpy(loader[i].bsymbolName, objCode + 6);
				for(int j = 6; j <= 10; j++){
					objCode[j] = 0; 
				}
					
                loader[i].aAddress = strtol(objCode, NULL, 16) + loader[i].address;
                fscanf(fileprog, "%s", objCode);
                loader[i].bAddress = strtol(objCode, NULL, 16) + loader[i].address;
                isBreak = 1;
				
			}
			if(isBreak == 1) break;
		}
		fclose(fileprog);
	}
	printf("%-20s%-15s%-11s%-10s\n", "Control section","Symbol name", "Address", "Length");
    for (int i = 0; i < 3; i++) {
        printf("%-35s", loader[i].controlSection);
        printf("%04X", loader[i].address);
		printf("%7c",' ');
        printf("%04X\n", loader[i].length);
		printf("%20c",' ');
        printf("%-10s", loader[i].asymbolName);
		printf("%5c",' ');
        printf("%04X\n", loader[i].aAddress);
		printf("%20c",' ');
        printf("%-10s", loader[i].bsymbolName);
		printf("%5c",' ');
        printf("%04X\n", loader[i].bAddress);
    }
    return 0;
	
}