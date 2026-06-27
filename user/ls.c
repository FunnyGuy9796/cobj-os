#include <printf.h>
#include <fs.h>

#define MAX_ENTRIES 64

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "";
    tar_entry_t entries[MAX_ENTRIES];
    int count = listdir(path, entries, MAX_ENTRIES);

    for (int i = 0; i < count; i++) {
        if (entries[i].type == '5')
            printf("%s/\n", entries[i].name);
        else
            printf("%s  (%d bytes)\n", entries[i].name, entries[i].size);
    }

    return 0;
}