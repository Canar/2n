//minimal cat for windows demonstrating appropriate binary mode-setting
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

int main() {
    char buffer[16384];
    int count;
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    while ((count = fread(buffer, 1, sizeof(buffer), stdin)) != 0)
        fwrite(buffer, 1, count, stdout);
    return 0;
}
