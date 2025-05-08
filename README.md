# Laporan Resmi Modul 3 Kelompok IT-07
## Anggota

| Nama 				| NRP		|
|-------------------------------|---------------|
| Muhammad Rakha Hananditya R.	| 5027241015 	|
| Zaenal Mustofa		| 5027241018 	|
| Mochkamad Maulana Syafaat	| 5027241021 	|

## • Soal  1

## • Soal 2
### • Pendahuluan
Tahun 2025, di tengah era perdagangan serba cepat, berdirilah sebuah perusahaan ekspedisi baru bernama RushGo. RushGo ingin memberikan layanan ekspedisi terbaik dengan 2 pilihan, Express (super cepat) dan Reguler (standar). Namun, pesanan yang masuk sangat banyak! Mereka butuh sebuah sistem otomatisasi pengiriman, agar agen-agen mereka tidak kewalahan menangani pesanan yang terus berdatangan. Kamu ditantang untuk membangun Delivery Management System untuk RushGo. 
Sistem ini terdiri dari dua bagian utama:
  - delivery_agent.c untuk agen otomatis pengantar Express
  - dispatcher.c untuk pengiriman dan monitoring pesanan oleh user  

Berikut ini program yang telah selesai dibuat :  
`delivery_agent.c`
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

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

SharedData *data;

void write_log(const char *agent, const char *name, const char *address) {
    FILE *log = fopen("delivery.log", "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] Express package delivered to %s in %s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, name, address);
    fclose(log);
}

void *agent_thread(void *arg) {
    char *agent = (char *)arg;
    while (1) {
        for (int i = 0; i < data->count; ++i) {
            if (data->orders[i].type == EXPRESS && data->orders[i].status == PENDING) {
                data->orders[i].status = DELIVERED;
                strcpy(data->orders[i].agent, agent);
                write_log(agent, data->orders[i].name, data->orders[i].address);
                sleep(1); // Simulasi waktu pengantaran
            }
        }
        sleep(2); // Cek ulang setiap 2 detik
    }
    return NULL;
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid >= 0) {
        printf("Inisialisasi shared memory dan baca CSV...\n");
        data = (SharedData *)shmat(shmid, NULL, 0);

        FILE *file = fopen("delivery_order.csv", "r");
        if (!file) {
            perror("Gagal membuka delivery_order.csv");
            return 1;
        }

        char line[256];
        data->count = 0;
        while (fgets(line, sizeof(line), file)) {
            char *name = strtok(line, ",");
            char *address = strtok(NULL, ",");
            char *type = strtok(NULL, "\n");

            if (name && address && type && data->count < MAX_ORDERS) {
                Order *o = &data->orders[data->count++];
                strncpy(o->name, name, sizeof(o->name));
                strncpy(o->address, address, sizeof(o->address));
                o->type = (strcmp(type, "Express") == 0) ? EXPRESS : REGULER;
                o->status = PENDING;
                strcpy(o->agent, "-");
            }
        }
        fclose(file);
    } else {
        shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
        data = (SharedData *)shmat(shmid, NULL, 0);
    }

    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, agent_thread, "AGENT A");
    pthread_create(&t2, NULL, agent_thread, "AGENT B");
    pthread_create(&t3, NULL, agent_thread, "AGENT C");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    return 0;
}
```
`dispatcher.c`
```c
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

```

## • Soal  3

## • Soal  4: Hunters System
### • Pendahuluan
Soal 4 terdiri dari dua program yaitu `hunter.c` dan `system.c` yang berbagi data antara satu dengan yang lainnya menggunakan mekanisme shared memory. Soal ini terdiri dari dua belas subsoal dimana sebelas subsoal pertama kita diperintahkan untuk menambahkan fitur pada sistem tracking hunter tersebut, sedangkan subsoal terakhir memerintahkan kita untuk membuat agar data pada shared memory tidak mengalami kebocoran. Adapun program `hunter` dan `system` menggunakan header file `shm_common.h` dalam pengoperasiannya untuk dapat berjalan, dimana tampilannya adalah sebagai berikut:

```c
#ifndef SHM_COMMON_H
#define SHM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50

struct Hunter {
    char username[50];
    int level;
    int exp;
    int atk;
    int hp;
    int def;
    int banned;
    key_t shm_key;
};

struct Dungeon {
    char name[50];
    int min_level;
    int exp;
    int atk;
    int hp;
    int def;
    key_t shm_key;
};

struct SystemData {
    struct Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    struct Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
    int current_notification_index;
};

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

#endif
```

Dimana penjelasan untuk setiap preprocessor yang secara spesifik digunakan pada program `hunter` dan `system`:

```c
#include <stdio.h>
```
1. Menyediakan elemen dari standard I/O yang berkaitan dengan fungsi menampilkan error menggunakan stderr, menampilkan output ke terminal menggunakan stdout, dan memanipulasi file. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<stdio.h>` adalah: `fprintf()`, macro `stderr`, macro `stdout`, struct `FILE`, `fopen()`, `snprintf()`, `fclose()`, `getchar()`, dan `scanf()`.
```c
#include <stdlib.h>
```
2. Menyediakan elemen dari standard library yang berkaitan dengan fungsi konversi tipe data dan manajemen control process. Adapun elemen yang digunakan program `hunter` dan `system` yang berkaitan dengan file header `<stdlib.h>` adalah: `exit()`, macro `EXIT_FAILURE`, macro `EXIT_SUCCESS`, `rand()`, dan `srand()`.

```c
```c
#include <string.h>
```
3. Menyediakan elemen yang berkaitan dengan fungsi memanipulasi tipe data `string`. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<string.h>` adalah: `strcmp()`, `strncpy()`, dan `memcpy()`.

```c
```c
#include <sys/ipc.h>
```
4. Menyediakan elemen yang berkaitan dengan interprocess communication atau IPC. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<sys/ipc.h>` adalah: `ftok()`.
```c
#include <sys/shm.h>
```
5. Menyediakan elemen yang berkaitan dengan proses pertukaran data menggunakan shared memory. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<sys/ipc.h>` adalah: `shmget()`, `shmat()`, `shmdt()`, dan `shmctl()`.
```c
#include <unistd.h>
```
6. Menyediakan elemen dari standard UNIX yang berkaitan dengan fungsi berinteraksi langsung dengan operating system. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<unistd.h>` adalah: `sleep()`.
```c
#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
```
7. Mendefinisikan macros `MAX_HUNTERS` dan `MAX_DUNGEONS` yang merepresentasikan limit banyaknya individual dungeon dan hunters pada sistem.
```c
struct Hunter {
    char username[50];
    int level;
    int exp;
    int atk;
    int hp;
    int def;
    int banned;
    key_t shm_key;
};
```
8. Mendeklarasikan struktur Hunter beserta data yang berkaitan dengannya seperti status level, exp, attack, hitpoints, defense, dan key khusus untuk mengakses data hunter di shared memory yang spesifik untuk hunter tersebut. Selain itu, struktur Hunter juga menyimpan status apakah suatu hunter diblokir oleh sistem atau tidak.
```c
struct Dungeon {
    char name[50];
    int min_level;
    int exp;
    int atk;
    int hp;
    int def;
    key_t shm_key;
};
```
9. Mendeklarasikan struktur Dungeon beserta data yang berkaitan dengannya seperti status level, exp, attack, hitpoints, defense, dan key khusus untuk mengakses data dungeon di shared memory yang spesifik untuk dungeon tersebut.
```c
struct SystemData {
    struct Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    struct Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
    int current_notification_index;
};
```
10. Mendeklarasikan struktur SystemData beserta data yang berkaitan dengannya seperti array yang berisi data semua hunter dan dungeon yang terdaftar dalam sistem, jumlah hunter dan dungeon yang terdapat pada sistem, serta indeks untuk menyimpan data mengenai sistem kendali notifikasi setiap hunter.
```c
key_t get_system_key() {
    return ftok("/tmp", 'S');
}
```
11. Merupakan function untuk membuat kunci sistem yang digunakan dalam proses pertukaran data menggunakan shared memory.
