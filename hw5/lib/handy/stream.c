/*
 * Handy - Streams
 *
 * Description:
 *     Implements a stream struct which reads from a file descriptor
 *     and returns char* pointers. The stream contains the text in its
 *     entirety, though it breaks it up into spaces. Each "word" is
 *     obtained via a call to "cn_stream_next(...)".
 *
 * Author:
 *     Clara Van Nguyen
 */

#include "handy.h"
#include "types.h"
#include "file.h"
#include "stream.h"

//File Stream Functions
CN_FSTREAM cn_fstream_init(FILE* fp) {
	CN_FSTREAM stream = (CN_FSTREAM) malloc(sizeof(struct cn_fstream));
	stream->strloc = fp;
	stream->val = NULL;

	return stream;
}

void cn_fstream_next(CN_FSTREAM stream) {
	//Almost equal to "fstrread()" in "strmanip.c"
	if (stream->val != NULL) {
		free(stream->val);
		stream->val = NULL;
	}

	stream->val = (char *) malloc(1);
	int data;
	cn_uint i = 0;
	stream->val[0] = '\0';

	//Make sure we don't run into a space as our first character
	do {
		data = fgetc(stream->strloc);
		if (data == '\0' || data == -1)
			break;
	}
	while (data == ' ' || data == '\r' || data == '\n' || data == '\t');

	//If we didn't get a null character, start reading in characters.
	if (data != '\0' && data != -1) {
		do {
			stream->val = (char *) realloc(stream->val, i + 1);
			stream->val[i++] = data;
			stream->val[i  ] = '\0';

			data = fgetc(stream->strloc);
		}
		while (data != ' ' && data != '\0' && data != '\r' && data != '\n' && data != '\t' && data != -1);
	}

	if (i == 0) {
		free(stream->val);
		stream->val = NULL;
		return;
	}
}

char* cn_fstream_get(CN_FSTREAM stream) {
	return stream->val;
}

void cn_fstream_free(CN_FSTREAM stream) {
	if (stream->val != NULL)
		free(stream->val);
	free(stream);
}

//Buffered File Stream Functions
CN_BFSTREAM cn_bfstream_init(char* fname) {
	CN_BFSTREAM stream = (CN_BFSTREAM) malloc(sizeof(struct cn_bfstream));

	size_t sz = get_file_size(fname);

	//Read entire file into buffer
	stream->str = (char *) malloc(sizeof(char) * (sz + 1));
	stream->val = NULL;
	stream->pos = 0;
	stream->len = sz;

	FILE* fp = fopen(fname, "rb");
	fread(stream->str, sizeof(char), sz, fp);
	fclose(fp);

	stream->str[sz] = '\0';

	return stream;
}

void cn_bfstream_next(CN_BFSTREAM stream) {
	if (stream->str == NULL)
		return; //Nice try
	if (stream->val == NULL)
		stream->pos = 0;

	int data;
	size_t sz = 0;
	//Go to the next available string.
	do {
		data = stream->str[stream->pos];
		stream->pos++;
		if (data == '\0' || data == -1)
			break;
	}
	while (data == ' ' || data == '\n' || data == '\t');

	if (data != '\0' && data != -1) {
		if (stream->val != NULL) {
			free(stream->val);
			stream->val = NULL;
		}
		stream->val = (char *) malloc(sizeof(char) * 1);
		stream->val[0] = '\0';

		do {
			stream->val = (char *) realloc(stream->val, ++sz + 1);
			stream->val[sz - 1] = data;
			stream->val[sz    ] = '\0';
			data = stream->str[stream->pos++];
		}
		while (data != ' ' && data != '\0' && data != '\n' && data != '\t' && data != -1);

		if (sz == 0 && data == '\0') {
			free(stream->val);
			stream->val = NULL;
			return;
		}
	}

	if (data == '\0') {
		if (stream->val != NULL) {
			free(stream->val);
			stream->val = NULL;
		}
	}
}

char* cn_bfstream_get(CN_BFSTREAM stream) {
	return stream->val;
}

void cn_bfstream_free(CN_BFSTREAM stream) {
	if (stream->val != NULL)
		free(stream->val);
	if (stream->str != NULL)
		free(stream->str);
	free(stream);
}

//String Stream Functions
CN_SSTREAM cn_sstream_init(char* ptr) {
	CN_SSTREAM stream = (CN_SSTREAM) malloc(sizeof(struct cn_sstream));
	stream->str = ptr;
	stream->val = NULL;
	stream->pos = 0;
	stream->len = strlen(ptr);

	return stream;
}

void cn_sstream_next(CN_SSTREAM stream) {
	if (stream->str == NULL)
		return; //Nice try
	if (stream->val == NULL)
		stream->pos = 0;

	int data;
	size_t sz = 0;
	//Go to the next available string.
	do {
		data = stream->str[stream->pos];
		stream->pos++;
		if (data == '\0' || data == -1)
			break;
	}
	while (data == ' ' || data == '\n' || data == '\t');

	if (data != '\0' && data != -1) {
		if (stream->val != NULL) {
			free(stream->val);
			stream->val = NULL;
		}
		stream->val = (char *) malloc(sizeof(char) * 1);
		stream->val[0] = '\0';

		do {
			stream->val = (char *) realloc(stream->val, ++sz + 1);
			stream->val[sz - 1] = data;
			stream->val[sz    ] = '\0';
			data = stream->str[stream->pos++];
		}
		while (data != ' ' && data != '\0' && data != '\n' && data != '\t' && data != -1);
	}
	else {
		if (stream->val != NULL)
			free(stream->val);
		stream->val = NULL;
	}
}

char* cn_sstream_get(CN_SSTREAM stream) {
	return stream->val;
}

void cn_sstream_free(CN_SSTREAM stream) {
	if (stream->val != NULL)
		free(stream->val);
	free(stream);
}
