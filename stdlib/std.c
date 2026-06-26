#include <stdio.h>
#include <stdlib.h>
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
