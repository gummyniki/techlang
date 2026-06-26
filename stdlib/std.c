#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void tec_print_int(int x)    { printf("%d", x); }
void tec_print_newline() { printf("\n"); }
void tec_print_float(float x){ printf("%f", x); }
void tec_print_string(char* s){ printf("%s", s); }
void tec_print_char(char c) { printf("%c", c); }
void tec_print_bool(bool b) { printf("%b", b); }
void tec_exit(int code)      { exit(code); }
int tec_readInt()           { int x; scanf("%d", &x); return x; }
float tec_readFloat() { float x; scanf("%f", &x); return x; };

float tec_sqrt(float x)      { return sqrtf(x); }

