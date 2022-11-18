#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE_LEN        256
#define ADDRESS_NUM_IN_PAGE  8
#define MAIN_MEM_PAGE_NUM    4
#define VIRTUAL_MEM_PAGE_NUM 16
#define POLICY_FIFO          0
#define POLICY_LRU           1

typedef struct _Page {
    char validBit;
    char dirtyBit;
    int  pageNum;
    int  value[ADDRESS_NUM_IN_PAGE];
    int  usedTime;
} Page;

Page g_mainMem[MAIN_MEM_PAGE_NUM];

Page g_pageTable[VIRTUAL_MEM_PAGE_NUM];

int g_usedTime = 0;

int g_poliy = POLICY_FIFO;

void init() {
    for (int i = 0; i < MAIN_MEM_PAGE_NUM; ++i) {
        g_mainMem[i].validBit = 0;
        g_mainMem[i].dirtyBit = 0;
        g_mainMem[i].pageNum = i;
        for (int j = 0; j < ADDRESS_NUM_IN_PAGE; ++j) {
            g_mainMem[i].value[j] = -1;
        }
        g_mainMem[i].usedTime = 0;
    }

    for (int i = 0; i < VIRTUAL_MEM_PAGE_NUM; ++i) {
        g_pageTable[i].validBit = 0;
        g_pageTable[i].dirtyBit = 0;
        g_pageTable[i].pageNum = i;
        for (int j = 0; j < ADDRESS_NUM_IN_PAGE; ++j) {
            g_pageTable[i].value[j] = -1;
        }
        g_pageTable[i].usedTime = 0;
    }
}

int parse_to_int(const char* str, int* value) {
    if (NULL == str || value == NULL) {
        return -1;
    }

    int tmp = 0;
    for (; *str != '\0'; ++str) {
        if (*str >= '0' && *str <= '9') {
            tmp = tmp * 10 + (*str - '0');
        } else {
            return -1;
        }
    }

    *value = tmp;
    return 0;
}

int select_main_mem() {
    int minUsedTime = g_usedTime;
    int selected = -1;
    for (int i = 0; i < MAIN_MEM_PAGE_NUM; ++i) {
        if (g_mainMem[i].validBit == 0) {
            return i;
        } else if (g_mainMem[i].usedTime < minUsedTime) {
            selected = i;
            minUsedTime = g_mainMem[i].usedTime;
        }
    }

    if (1 == g_mainMem[selected].dirtyBit) {
        int virtualPageNum = g_mainMem[selected].pageNum;
        for (int i = 0; i < ADDRESS_NUM_IN_PAGE; ++i) {
            g_pageTable[virtualPageNum].value[i] = g_mainMem[selected].value[i];
        }
        g_pageTable[virtualPageNum].validBit = 0;
    }
    
    return selected;
}

void handle_read(int virtualAddr) {
    int virtualPageNum = virtualAddr / 8;
    if (virtualPageNum >= VIRTUAL_MEM_PAGE_NUM) {
        printf("virtual_addr: %d is invalid\n", virtualAddr);
        return;
    }

    if (0 == g_pageTable[virtualPageNum].validBit) {
        printf("A Page Fault Has Occurred\n");
        int mainPageNum = select_main_mem();
        g_mainMem[mainPageNum].validBit = 1;
        g_mainMem[mainPageNum].pageNum = virtualPageNum;
        for (int i = 0; i < ADDRESS_NUM_IN_PAGE; ++i) {
            g_mainMem[mainPageNum].value[i] = g_pageTable[virtualPageNum].value[i]; 
        }
        g_mainMem[mainPageNum].usedTime = g_usedTime++;
        g_pageTable[virtualPageNum].validBit = 1;
        g_pageTable[virtualPageNum].pageNum = mainPageNum;
    }

    int mainPageNum = g_pageTable[virtualPageNum].pageNum;
    int offset = virtualAddr % 8;
    printf("%d\n", g_mainMem[mainPageNum].value[offset]);
    if (POLICY_LRU == g_poliy) {
        g_mainMem[mainPageNum].usedTime = g_usedTime++;
    }
}

void handle_write(int virtualAddr, int num) {
    int virtualPageNum = virtualAddr / 8;
    if (virtualPageNum >= VIRTUAL_MEM_PAGE_NUM) {
        printf("virtual_addr: %d is invalid\n", virtualAddr);
        return;
    }

    if (0 == g_pageTable[virtualPageNum].validBit) {
        printf("A Page Fault Has Occurred\n");
        int mainPageNum = select_main_mem();
        g_mainMem[mainPageNum].validBit = 1;
        g_mainMem[mainPageNum].pageNum = virtualPageNum;
        for (int i = 0; i < ADDRESS_NUM_IN_PAGE; ++i) {
            g_mainMem[mainPageNum].value[i] = g_pageTable[virtualPageNum].value[i]; 
        }
        g_mainMem[mainPageNum].usedTime = g_usedTime++;
        g_pageTable[virtualPageNum].validBit = 1;
        g_pageTable[virtualPageNum].pageNum = mainPageNum;
    }

    int mainPageNum = g_pageTable[virtualPageNum].pageNum;
    int offset = virtualAddr % 8;
    g_mainMem[mainPageNum].value[offset] = num;
    g_mainMem[mainPageNum].dirtyBit = 1;
    if (POLICY_LRU == g_poliy) {
        g_mainMem[mainPageNum].usedTime = g_usedTime++;
    }
}

void handle_showmain(int pageNum) {
    if (pageNum >= MAIN_MEM_PAGE_NUM) {
        printf("ppn: %d is invalid\n", pageNum);
        return;
    }

    for (int i = 0; i < ADDRESS_NUM_IN_PAGE; ++i) {
        printf("%d: %d\n", ADDRESS_NUM_IN_PAGE * pageNum + i, 
               g_mainMem[pageNum].value[i]);
    }
}

void handle_showptable() {
    for (int i = 0; i < VIRTUAL_MEM_PAGE_NUM; ++i) {
        printf("%d:%hhd:%hhd:%d\n", i, g_pageTable[i].validBit, 
               g_pageTable[i].validBit ? g_mainMem[g_pageTable[i].pageNum].dirtyBit : 0, 
               g_pageTable[i].validBit ? g_pageTable[i].pageNum : i);
    }
}

void usage(const char* cmd) {
    printf("Usage:\n"
           "%s\n"
           "%s FIFO\n"
           "%s LRU\n", 
           cmd, cmd, cmd);
}

int main(int argc, char** argv) {
    if (argc > 2) {
        usage(argv[0]);
        return -1;
    }

    if (2 == argc) {
        if (strcmp(argv[1], "FIFO") == 0) {
            g_poliy = POLICY_FIFO;
        } else if (strcmp(argv[1], "LRU") == 0) {
            g_poliy = POLICY_LRU;
        } else {
            usage(argv[0]);
            return -1;
        }
    }

    init();
    char line[MAX_LINE_LEN];
    while (1) {
        printf("> ");
        if (NULL == fgets(line, MAX_LINE_LEN, stdin)) {
            printf("fgets failed errno[%d] errmsg[%s]\n", errno, strerror(errno));
            return -1;
        }

        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        char* tok = strtok(line, " ");
        if (NULL == tok) {
            printf("invalid input\n");
            continue;
        }

        if (strcmp(tok, "read") == 0) {
            tok = strtok(NULL, " ");
            if (NULL == tok) {
                printf("invalid input\n");
                continue;
            }

            int virtualAddr = 0;
            if (0 != parse_to_int(tok, &virtualAddr)) {
                printf("invalid input\n");
                continue;
            }

            tok = strtok(NULL, " ");
            if (NULL != tok) {
                printf("invalid input\n");
                continue;
            }

            handle_read(virtualAddr);
        } else if (strcmp(tok, "write") == 0) {
            tok = strtok(NULL, " ");
            if (NULL == tok) {
                printf("invalid input\n");
                continue;
            }

            int virtualAddr = 0;
            if (0 != parse_to_int(tok, &virtualAddr)) {
                printf("invalid input\n");
                continue;
            }

            tok = strtok(NULL, " ");
            if (NULL == tok) {
                printf("invalid input\n");
                continue;
            }

            int num = 0;
            if (0 != parse_to_int(tok, &num)) {
                printf("invalid input\n");
                continue;
            }

            tok = strtok(NULL, " ");
            if (NULL != tok) {
                printf("invalid input\n");
                continue;
            }

            handle_write(virtualAddr, num);
        } else if (strcmp(tok, "showmain") == 0) {
            tok = strtok(NULL, " ");
            if (NULL == tok) {
                printf("invalid input\n");
                continue;
            }

            int pageNum = 0;
            if (0 != parse_to_int(tok, &pageNum)) {
                printf("invalid input\n");
                continue;
            }

            tok = strtok(NULL, " ");
            if (NULL != tok) {
                printf("invalid input\n");
                continue;
            }

            handle_showmain(pageNum);
        } else if (strcmp(tok, "showptable") == 0) {
            tok = strtok(NULL, " ");
            if (NULL != tok) {
                printf("invalid input\n");
                continue;
            }

            handle_showptable();
        } else if (strcmp(tok, "quit") == 0) {
            tok = strtok(NULL, " ");
            if (NULL != tok) {
                printf("invalid input\n");
                continue;
            }
            break;
        } else {
            printf("invalid input\n");
        }        
    }
    
    return 0;
}
