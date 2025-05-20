#include <cstdio>
#include <cstdlib>
#include "HideImport.hpp"

using printf_t = int (*)(const char*, ...);
using fopen_t = FILE* (*)(const char*, const char*);
using fclose_t = int (*)(FILE*);

static printf_t my_printf = reinterpret_cast<printf_t>(HI_GET("libc.so", "printf"));
static fopen_t my_fopen = reinterpret_cast<fopen_t>(HI_GET("libc.so", "fopen"));
static fclose_t my_fclose = reinterpret_cast<fclose_t>(HI_GET("libc.so", "fclose"));

static void check_hook() {
    my_printf("[+] printf : %s\n", IS_FUNCTION_HOOKED(my_printf) ? "hooked" : "ok");
    my_printf("[+] fopen : %s\n", IS_FUNCTION_HOOKED(my_fopen)  ? "hooked" : "ok");
}

int main() {
    check_hook();

    FILE* f = my_fopen("/proc/self/cmdline", "r");
    if (!f) {
        my_printf("[-] fopen failed\n");
        return 1;
    }
    char buf[64] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    my_fclose(f);
  
    my_printf("[+] cmdline: %s\n", buf);
    return 0;
}
