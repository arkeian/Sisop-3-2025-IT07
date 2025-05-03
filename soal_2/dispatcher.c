#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_KEY 1234
#define MAX_ORDERS 100

typedef enum {
    EXPRESS,
    REGULER
} OrderType;

typedef enum {
    PENDING,
    DELIVERED
} OrderStatus;

typedef struct {
    char name[64];
    char address[128];
    OrderType type;
    OrderStatus status;
    char agent[64];
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
} SharedData;

void write_log(const char *agent, const char *name, const char *address) {
    FILE *log = fopen("delivery.log", "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [AGENT %s] Reguler package delivered to %s in %s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, name, address);
    fclose(log);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n  ./dispatcher -deliver [Nama]\n  ./dispatcher -status [Nama]\n  ./dispatcher -list\n");
        return 1;
    }

    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("Shared memory not found");
        return 1;
    }
    SharedData *data = (SharedData *)shmat(shmid, NULL, 0);

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        char *name = argv[2];
        for (int i = 0; i < data->count; i++) {
            if (strcmp(data->orders[i].name, name) == 0 &&
                data->orders[i].type == REGULER &&
                data->orders[i].status == PENDING) {
                data->orders[i].status = DELIVERED;

                const char *username = getenv("USER");
                if (!username) username = getenv("LOGNAME");
                if (!username) username = "Unknown";
                strncpy(data->orders[i].agent, username, sizeof(data->orders[i].agent));

                write_log(data->orders[i].agent, data->orders[i].name, data->orders[i].address);
                printf("Order %s delivered.\n", name);
                return 0;
            }
        }
        printf("Order not found or already delivered.\n");

    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        char *name = argv[2];
        for (int i = 0; i < data->count; i++) {
            if (strcmp(data->orders[i].name, name) == 0) {
                if (data->orders[i].status == DELIVERED) {
                    printf("Status for %s: Delivered by Agent %s\n", name, data->orders[i].agent);
                } else {
                    printf("Status for %s: Pending\n", name);
                }
                return 0;
            }
        }
        printf("Order not found.\n");

    } else if (strcmp(argv[1], "-list") == 0) {
        for (int i = 0; i < data->count; i++) {
            printf("%s - %s\n", data->orders[i].name,
                   data->orders[i].status == DELIVERED ? "Delivered" : "Pending");
        }

    } else {
        printf("Invalid command.\n");
    }

    return 0;
}

