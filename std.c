#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void tec_print_int(int x)    {printf("%d\n", x); }
void tec_print_float(float x){ printf("%f\n", x); }
void tec_print_string(char* s){ printf("%s\n", s); }
void tec_exit(int code)      { exit(code); }
int  tec_readInt()           { int x; scanf("%d", &x); return x; }
float tec_sqrt(float x)      { return sqrtf(x); }
