#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// printing
void tec_print_int(int x)    { printf("%d", x); }
void tec_print_newline() { printf("\n"); }
void tec_print_float(float x){ printf("%f", (double)x); }
void tec_print_string(char* s){ printf("%s", s); }
void tec_print_char(char c) { printf("%c", c); }
void tec_print_bool(bool b) { printf("%b", b); }

// exiting
void tec_exit(int code)      { exit(code); }

// reading
int tec_readInt()           { int x; scanf("%d", &x); return x; }
float tec_readFloat() { float x; scanf("%f", &x); return x; };

// math
float tec_sqrt(float x)      { return sqrtf(x); }


// file operations
FILE* tec_file_open(const char* path, const char* mode) {
    return fopen(path, mode);
}

void tec_file_close(FILE* file) {
    fclose(file);
}

void tec_file_write(FILE* file, const char* text) {
    fprintf(file, "%s", text);
}

char* tec_file_read_line(FILE* file) {
    static char buffer[1024];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        return "";
    }
    return buffer;
}

int tec_file_eof(FILE* file) {
    return feof(file);
}

void tec_file_delete(const char* path) {
    remove(path);
}

// string concatenation


char* tec_concat(const char* a, const char* b) {
    char* result = (char *)malloc(strlen(a) + strlen(b) + 1);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

int tec_string_length(const char* s) {
    return strlen(s);
}

int tec_string_equals(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

char* tec_string_substring(const char* s, int start, int end) {
    int len = end - start;
    char* result = (char *)malloc(len + 1);
    strncpy(result, s + start, len);
    result[len] = '\0';
    return result;
}
