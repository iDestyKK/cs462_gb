/*
 * HandyC - String Manipulation
 * 
 * Description:
 *     Functions to manipulate strings and write data to or read data from.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#include "strmanip.h"

/*
	fstrread (File String Read)
	
	Reads (one byte at a time) characters and puts it into a char* array.
	Realloc is called constantly until the stream hits a space, newline,
	or null-terminated character. This is non-buffered and is a very slow
	implementation.
*/

char* fstrread(FILE* _file_desc) {
	char* str = (char *) malloc(1);
	int data;
	cn_uint i = 0;
	str[0] = '\0';

	//Make sure we don't run into a space as our first character
	do {
		data = fgetc(_file_desc);
		if (data == '\0' || data == -1)
			break;
	}
	while (data == ' ' || data == '\n' || data == '\t');
	
	//If we didn't get a null character, start reading in characters.
	if (data != '\0' && data != -1) {
		do {
			str = (char *) realloc(str, i + 1);
			str[i++] = data;
			str[i  ] = '\0';

			data = fgetc(_file_desc);
		}
		while (data != ' ' && data != '\0' && data != '\n' && data != '\t' && data != -1);
	}

	if (i == 0){
		free(str);
		return NULL;
	}

	return str;
}

/*
	fbufstr (File Buffer to String)
	
	Uses ftell to get size of file, then reads the entire file into a char
	buffer for later use. The "val" is freed if it was already declared.
	The function returns the number of bytes read. It should be noted that
	there is an additional byte holding null termination. If we don't want
	this additional byte, run fbufchar().
*/

size_t fbufstr(FILE* _file_desc, cn_byte* val) {
	size_t size, i;
	//if (val != NULL)
	//	free(val);
	
	//Get file size
	fseek(_file_desc, 0, SEEK_END);
	size = ftell(_file_desc);
	fseek(_file_desc, 0, SEEK_SET);

	//Create a buffer
	val = (cn_byte *) calloc(size + 1, sizeof(cn_byte));

	//Read into buffer
	for (i = 0; i < size; i++)
		val[i] = fgetc(_file_desc);
	return i;
	//return fread(val, 1, size, _file_desc);
}

/*
	fbufchar (File Buffer to Char Array)
	
	Uses ftell to get size of file, then reads the entire file into a char
	buffer for later use. The "val" is freed if it was already declared.
	The function returns the number of bytes read. This function is
	identical to fbufstr() except it doesn't store an additional byte that
	is null terminated.
*/

size_t fbufchar(FILE* _file_desc, cn_byte* val) {
	size_t size, i;
	//if (val != NULL)
	//	free(val);
	
	//Get file size
	fseek(_file_desc, 0, SEEK_END);
	size = ftell(_file_desc);
	fseek(_file_desc, 0, SEEK_SET);

	//Create a buffer
	val = (cn_byte *) calloc(size, sizeof(cn_byte));

	//Read into buffer
	for (i = 0; i < size; i++)
		val[i] = fgetc(_file_desc);
	return i;
	//return fread(val, 1, size, _file_desc);
}
