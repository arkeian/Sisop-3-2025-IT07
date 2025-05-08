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

Soal 4 terdiri dari dua program yaitu `hunter.c` dan `system.c` yang berbagi data antara satu dengan yang lainnya menggunakan mekanisme shared memory. Soal ini terdiri dari dua belas subsoal dimana sebelas subsoal pertama kita diperintahkan untuk menambahkan fitur pada sistem tracking hunter tersebut, sedangkan subsoal terakhir memerintahkan kita untuk membuat agar data pada segmen shared memory tersebut tidak mengalami kebocoran. Adapun program `hunter` dan `system` menggunakan header file `shm_common.h` dalam pengoperasiannya untuk dapat berjalan, dimana tampilannya adalah sebagai berikut:

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
#include <string.h>
```
3. Menyediakan elemen yang berkaitan dengan fungsi memanipulasi tipe data `string`. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<string.h>` adalah: `strcmp()`, `strncpy()`, dan `memcpy()`.

```c
#include <sys/ipc.h>
```
4. Menyediakan elemen yang berkaitan dengan interprocess communication atau IPC. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<sys/ipc.h>` adalah: `ftok()`.

```c
#include <sys/shm.h>
```
5. Menyediakan elemen yang berkaitan dengan proses mengakses data menggunakan mekanisme shared memory. Adapun elemen yang digunakan pada program `hunter` dan `system` yang berkaitan dengan file header `<sys/ipc.h>` adalah: `shmget()`, `shmat()`, `shmdt()`, dan `shmctl()`.

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
8. Mendeklarasikan struktur Hunter beserta data yang berkaitan dengannya seperti status level, exp, attack, hitpoints, defense, dan key khusus untuk mengakses data hunter di segmen shared memory yang spesifik untuk hunter tersebut. Selain itu, struktur Hunter juga menyimpan status apakah suatu hunter diblokir oleh sistem atau tidak.

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
9. Mendeklarasikan struktur Dungeon beserta data yang berkaitan dengannya seperti status level, exp, attack, hitpoints, defense, dan key khusus untuk mengakses data dungeon di segmen shared memory yang spesifik untuk dungeon tersebut.

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
11. Merupakan function untuk membuat key sistem yang digunakan dalam proses mengakses data menggunakan mekanisme shared memory.

### • Soal  4.A: Pengantar `hunter.c` dan `system.c`

Pada subsoal 4.A: Pengantar `hunter.c` dan `system.c`, kita diperintahkan untuk membuat dua program yang nantinya akan berinteraksi antara satu dengan yang lainnya, yaitu program `hunter.c` dan `system.c`. Dimana `system.c` merupakan program untuk mengelola dan menyimpan data mengenai hunter-hunter yang ada pada `hunter.c`. Adapun tampilan function `main()` untuk masing-masing program adalah sebagai berikut:

#### a. Soal 4.A.1: `hunter.c`

```c
int main() {
    FILE *huntfile = fopen("/tmp/hunter", "a");
    if (huntfile == NULL) {
        fprintf(stderr, "Error: Cannot create /tmp/dungeon\n");
        exit(EXIT_FAILURE);
    }
    fclose(huntfile);

    int opt;
    do {
        fprintf(stdout, "\e[H\e[2J\e[3J");
        fprintf(stdout, "=== HUNTER MENU ===\n1. Register\n2. Login\n3. Exit\nChoice: ");
        
        if (scanf("%d", &opt) != 1) {
            opt = -1;
            while (getchar() != '\n');
        }
        switch (opt) {
        case 1:
            regist_the_hunters();
            break;
        case 2:
            login_registered_hunters();
            break;
        case 3:
            break;
        default:
            fprintf(stderr, "Error: Unknown Option\n");
            sleep(3);
        }
    } while (opt != 3);
    exit(EXIT_SUCCESS);
}
```

Dimana langkah implementasinya:

```c
int main() {
	...
}
```
1. Merupakan deklarasi function `main()`.

```c
FILE *huntfile = fopen("/tmp/hunter", "a");
if (huntfile == NULL) {
    fprintf(stderr, "Error: Cannot create /tmp/dungeon\n");
    exit(EXIT_FAILURE);
}
```
2. Membuat file `/tmp/hunter` jika tidak ada yang digunakan dalam proses membuat key dengan konversi pathname ke dalam sebuah key yang digunakan untuk mengakses segmen shared memory yang khusus untuk setiap hunter menggunakan function `ftok()`. Apabila tidak ditemukan atau tidak dapat membuka `/tmp/hunter`, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
fclose(huntfile);
```
3. Menutup kembali file `/tmp/hunter`.

```c
int opt;
```
4. Mendeklarasikan variabel, dimana:
- `opt`: untuk menyimpan data opsi yang dipilih oleh user saat menjalankan program `hunter`.

```c
do {
    ...    
} while (opt != 3);
```
4. Merupakan suatu do-while loop untuk menampilkan UI pada layar terminal kepada user selama user tidak memilih opsi ketiga yang mengeluarkan user dari program.

```c
fprintf(stdout, "\e[H\e[2J\e[3J");
```
5. Membersihkan layar terminal. Memiliki peran yang sama layaknya command `clear`.

```c
fprintf(stdout, "=== HUNTER MENU ===\n1. Register\n2. Login\n3. Exit\nChoice: ");
```
6. Mengoutput UI ke layar terminal yang menampilkan pilihan opsi yang dapat diplih oleh user.

```c
if (scanf("%d", &opt) != 1) {
    opt = -1;
    while (getchar() != '\n');
}
```
7. Mencoba untuk mendapatkan input dari user dalam bentuk integer. Apabila user menginput suatu karakter yang bukan merupakan suatu integer, maka opsi akan diinsialisasi dengan `-1` dan membersihkan buffer input dari karakter yang tidak sesuai tersebut.

```c
switch (opt) {
case 1:
    regist_the_hunters();
    break;
case 2:
    login_registered_hunters();
    break;
case 3:
    break;
default:
    fprintf(stderr, "Error: Unknown Option\n");
    sleep(3);
}
```
8. Mengarahkan program ke function yang sesuai berdasarkan pada input yang diberikan oleh user pada variabel `opt`. Jika value di dalam `opt` merupakan value yang bukan antara 1, 2, atau 3, maka program akan melempar sebuah error ke stderr yang akan ditampilkan ke user dan memberikan jeda tiga detik agar user dapat membaca error tersebut.

```c
exit(EXIT_SUCCESS);
```
9. Jika user memilih opsi ketiga, maka do-while loop akan berhenti. Setelah itu, program dinyatakan berhasil dieksekusi dan keluar.

#### b. Soal 4.A.2: `system.c`

```c
int main() {
    srand(time(NULL));

    FILE *dngnfile = fopen("/tmp/dungeon", "a");
    if (dngnfile == NULL) {
        fprintf(stderr, "Error: Cannot create /tmp/dungeon\n");
        exit(EXIT_FAILURE);
    }
    fclose(dngnfile);

    key_t key;
    int shmid;
    
    if ((key = get_system_key()) == -1) {
        fprintf(stderr, "Error: Fail to create shared memory key\n");
        exit(EXIT_FAILURE);
    }

    if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to create shared memory\n");
        exit(EXIT_FAILURE);
    }

    if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach system data to shared memory\n");
        exit(EXIT_FAILURE);
    }

    if (sys->num_hunters < 0 || sys->num_hunters > MAX_HUNTERS) {
        sys->num_hunters = 0;
        sys->num_dungeons = 0;
        sys->current_notification_index = 0;
    }

    int opt;
    do {
        fprintf(stdout, "\e[H\e[2J\e[3J");
        fprintf(stdout, "=== SYSTEM MENU ===\n1. Hunter Info\n2. Dungeon Info\n3. Generate Dungeon\n4. Ban/Unban Hunter\n5. Reset Hunter\n6. Exit\nChoice: ");
    
        if (scanf("%d", &opt) != 1) {
            opt = -1;
            while (getchar() != '\n');
        }
        switch (opt) {
        case 1:
            info_of_all_hunters();
            break;
        case 2:
            info_of_all_dungeons();
            break;
        case 3:
            generate_random_dungeons();
            break;
        case 4:
            ban_hunter_because_hunter_bad();
            break;
        case 5:
            reset_the_hunters_stat_back_to_default();
            break;
        case 6:
            break;
        default:
            fprintf(stderr, "Error: Unknown Option\n");
            sleep(3);
        }
    } while (opt != 6);
    
    
    for (int i = 0; i < sys->num_hunters; i++) {
        shmctl(sys->hunters[i].shm_key, IPC_RMID, NULL);
    }
    for (int i = 0; i < sys->num_dungeons; i++) {
        shmctl(sys->dungeons[i].shm_key, IPC_RMID, NULL);
    }
    shmdt(sys);
    shmctl(shmid, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}
```

Dimana langkah implementasinya:

```c
int main() {
	...
}
```
1. Merupakan deklarasi function `main()`.

```c
srand(time(NULL));
```
2. Menginisialisasi proses seeding suatu angka pseudo-random yang nantinya akan digunakan oleh function `rand()`.

```c
FILE *dngnfile = fopen("/tmp/dungeon", "a");
if (dngnfile == NULL) {
	fprintf(stderr, "Error: Cannot create /tmp/dungeon\n");
	exit(EXIT_FAILURE);
}
```
3. Membuat file `/tmp/dungeon` jika tidak ada yang digunakan dalam proses membuat key dengan konversi pathname ke dalam sebuah key yang digunakan untuk mengakses segmen shared memory yang khusus untuk setiap dungeon menggunakan function `ftok()`. Apabila tidak ditemukan atau tidak dapat membuka `/tmp/dungeon`, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
fclose(dngnfile);
```
4. Menutup kembali file `/tmp/dungeon`.

```c
key_t key;
int shmid;
```
5. Mendeklarasikan variabel, dimana:
- `key`: untuk menyimpan data key utama yang digunakan untuk mengidentifikasi dan mengelola segmen shared memory mana yang akan digunakan bersama oleh program `hunter` dan `system`.
- `shmid`: untuk menyimpan data mengenai ID segmen shared memory yang digunakan oleh program `hunter` dan `system` untuk mengakses data.

```c
if ((key = get_system_key()) == -1) {
	fprintf(stderr, "Error: Fail to create shared memory key\n");
	exit(EXIT_FAILURE);
}
```
6. Mendapatkan value dari variabel key yang diambil dari function `get_system_key()` yang tertera pada header file. Apabila gagal untuk mendapatkan key-nya, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
	fprintf(stderr, "Error: Fail to create shared memory\n");
	exit(EXIT_FAILURE);
}
```
7. Function `shmget()` membuat segmen shared memory dengan key yang telah dibuat dan menyimpan data ID dari segmen shared memory tersebut ke dalam variabel `shmid`. Jika segmen shared memory dengan key tersebut belum ada, maka akan dibuat segmen shared memory yang baru menggunakan `IPC_CREAT` dengan izin read-write untuk semua user. Jika segmen sudah ada, maka function hanya perlu mengambil ID segmen shared memory yang sudah dibuat sebelumnya. Terakhir, apabila proses pembuatan segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
	fprintf(stderr, "Error: Fail to attach system data to shared memory\n");
	exit(EXIT_FAILURE);
}
```
8. Meng-attach segmen shared memory menggunakan function `shmat()` ke alamat memori yang dialokasikan ke program `system` yang sedang berjalan sesuai dengan ID segmen shared memory yang telah diberikan. Apabila proses meng-attach segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
if (sys->num_hunters < 0 || sys->num_hunters > MAX_HUNTERS) {
	sys->num_hunters = 0;
	sys->num_dungeons = 0;
	sys->current_notification_index = 0;
}
```
9. Jika program `system` dijalankan untuk pertama kalinya atau jumlah individual hunters yang terdaftar pada sistem melebihi batas limit, maka program akan menginisialisasi value untuk variabel jumlah hunters dan dungeons, serta indeks notifikasi untuk menyimpan data mengenai sistem kendali notifikasi setiap hunter ke value `0`.

```c
int opt;
```
10. Mendeklarasikan variabel, dimana:
- `opt`: untuk menyimpan data opsi yang dipilih oleh admin saat menjalankan program `hunter`.

```c
do {
    ...    
} while (opt != 6);
```
11. Merupakan suatu do-while loop untuk menampilkan UI pada layar terminal kepada user selama admin tidak memilih opsi keenam yang mengeluarkan admin dari program.

```c
fprintf(stdout, "\e[H\e[2J\e[3J");
```
12. Membersihkan layar terminal. Memiliki peran yang sama layaknya command `clear`.

```c
fprintf(stdout, "=== SYSTEM MENU ===\n1. Hunter Info\n2. Dungeon Info\n3. Generate Dungeon\n4. Ban/Unban Hunter\n5. Reset Hunter\n6. Exit\nChoice: ");
```
13. Mengoutput UI ke layar terminal yang menampilkan pilihan opsi yang dapat diplih oleh admin.

```c
if (scanf("%d", &opt) != 1) {
    opt = -1;
    while (getchar() != '\n');
}
```
14. Mencoba untuk mendapatkan input dari admin dalam bentuk integer. Apabila admin menginput suatu karakter yang bukan merupakan suatu integer, maka opsi akan diinsialisasi dengan `-1` dan membersihkan buffer input dari karakter yang tidak sesuai tersebut.

```c
switch (opt) {
	case 1:
		info_of_all_hunters();
		break;
	case 2:
		info_of_all_dungeons();
		break;
	case 3:
		generate_random_dungeons();
		break;
	case 4:
		ban_hunter_because_hunter_bad();
		break;
	case 5:
		reset_the_hunters_stat_back_to_default();
		break;
	case 6:
		break;
	default:
		fprintf(stderr, "Error: Unknown Option\n");
		sleep(3);
}
```
15. Mengarahkan program ke function yang sesuai berdasarkan pada input yang diberikan oleh admin pada variabel `opt`. Jika value di dalam `opt` merupakan value yang bukan antara 1, 2, 3, 4, 5, atau 6 maka program akan melempar sebuah error ke stderr yang akan ditampilkan ke admin dan memberikan jeda tiga detik agar admin dapat membaca error tersebut.

```c
for (int i = 0; i < sys->num_hunters; i++) {
	shmctl(sys->hunters[i].shm_key, IPC_RMID, NULL);
}
for (int i = 0; i < sys->num_dungeons; i++) {
	shmctl(sys->dungeons[i].shm_key, IPC_RMID, NULL);
}
shmdt(sys);
shmctl(shmid, IPC_RMID, NULL);
```
16. Merupakan bagian dari subsoal 4.L: Destruksi Shared Memory.

```c
exit(EXIT_SUCCESS);
```
17. Jika admin memilih opsi keenam, maka do-while loop akan berhenti. Setelah itu, program dinyatakan berhasil dieksekusi dan keluar.
