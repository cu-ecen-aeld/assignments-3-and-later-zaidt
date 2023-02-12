#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char **argv) 
{
    int ret = 0;
    openlog(NULL, 0, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Invalid params");
        ret = 1;
        goto end;
    }

    const char* filename = argv[1];
    const char* str = argv[2];

    // printf("Filename=%s str=%s\n", filename, str);

    FILE *file = fopen(filename, "w");

    if (!file){
        syslog(LOG_ERR, "Cannot open file");
        ret = 1;
        goto end;
    }

    fwrite(str, strlen(str), 1, file);
    fclose(file);
end:
    closelog();
    return ret;
}
