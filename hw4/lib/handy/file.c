/*
 * Handy - File Management
 *
 * Description:
 *     Includes functions which involve file I/O and management.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#include "file.h"

cn_bool file_exists(char* fname) {
	struct stat buffer;
	return (stat(fname, &buffer) == 0) ? CN_TRUE : CN_FALSE;
}

size_t get_file_size(char* fname) {
	size_t pos;
	FILE* fp = fopen(fname, "rb");
	fseek(fp, 0, SEEK_END);
	pos = ftell(fp);
	fclose(fp);

	return pos;
}

cn_byte array_to_file(char** lines, const char* fname) {
	FILE *op;
	op = fopen(fname, "w");

	cn_uint i = 0;
	for (; lines[i] != NULL; i++)
		fwrite(lines[i], sizeof(cn_byte), strlen(lines[i]), op),
			fputc('\n', op);

	fclose(op);
	return 1;
}

char** file_to_array(const char* fname) {
	//Going C style
	cn_uint lc = 0,
	        fs,
	        i, a, b, cl;

	char** lines;
	char* buffer;

	FILE *fp;
	fp = fopen(fname, "rb");
	
	//Get file size
	fseek(fp, 0, SEEK_END);
	fs = ftell(fp);
	fseek(fp, 0, SEEK_SET); //ALT: rewind()
	
	//Read into buffer
	buffer = (char *) malloc(sizeof(char) * fs);
	fread(buffer, sizeof(char), fs, fp);

	i = 0;
	for (; i < fs; i++) {
		if (buffer[i] == '\n')
			lc++;
	}
	lc++;
	
	//Finally, read the lines into a buffered array of arrays.
	lines = (char **) malloc(sizeof(char *) * lc);
	a = 0;
	cl = 0;
	for (; a < fs; a++) {
		b = a;
		for (;; b++) {
			if (buffer[b] == '\n' || buffer[b] == '\0')
				break;
		}
		lines[cl] = (char *) malloc(sizeof(char) * ((b - a) + 1));
		memcpy(lines[cl], buffer + a, (b - a));
		lines[cl++][b - a] = '\0';
		a += (b - a);
	}
	lines[cl] = NULL;
	free(buffer);
	fclose(fp);

	return lines;
}
