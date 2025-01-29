#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]){
	if (argc != 3){
	       syslog(LOG_ERR, "Error: Two areguments are required. Usage %s <writefile> <writestr>", argv[0]);
	       return 1;
        }
        // Capture arguments into variables 
	const char *writefile = argv[1];
        const char *writestr = argv[2];

        // Open the file and write the string value to it
	FILE *file = fopen(writefile, "w");
        if (file == NULL){
		syslog(LOG_ERR, "Error: Couldn't open the file %s", writefile);
		return 1;
	}
	fprintf(file, "%s\n", writestr);
	fclose(file);
	syslog(LOG_INFO, "Info: Teh string was written to %s", writefile);
	return 0;
}

