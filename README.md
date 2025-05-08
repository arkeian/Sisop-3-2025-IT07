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
struct SystemData *sys;

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
struct SystemData *sys;
```
1. Mendeklarasikan variabel, dimana:
- `sys`: struktur SystemData yang terhubung pada segmen shared memory utama sistem.

```c
int main() {
	...
}
```
2. Merupakan deklarasi function `main()`.

```c
srand(time(NULL));
```
3. Menginisialisasi proses seeding suatu angka pseudo-random yang nantinya akan digunakan oleh function `rand()`.

```c
FILE *dngnfile = fopen("/tmp/dungeon", "a");
if (dngnfile == NULL) {
	fprintf(stderr, "Error: Cannot create /tmp/dungeon\n");
	exit(EXIT_FAILURE);
}
```
4. Membuat file `/tmp/dungeon` jika tidak ada yang digunakan dalam proses membuat key dengan konversi pathname ke dalam sebuah key yang digunakan untuk mengakses segmen shared memory yang khusus untuk setiap dungeon menggunakan function `ftok()`. Apabila tidak ditemukan atau tidak dapat membuka `/tmp/dungeon`, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
fclose(dngnfile);
```
5. Menutup kembali file `/tmp/dungeon`.

```c
key_t key;
int shmid;
```
6. Mendeklarasikan variabel, dimana:
- `key`: untuk menyimpan data key utama yang digunakan untuk mengidentifikasi dan mengelola segmen shared memory mana yang akan digunakan bersama oleh program `hunter` dan `system`.
- `shmid`: untuk menyimpan data mengenai ID segmen shared memory yang digunakan oleh program `hunter` dan `system` untuk mengakses data.

```c
if ((key = get_system_key()) == -1) {
	fprintf(stderr, "Error: Fail to create shared memory key\n");
	exit(EXIT_FAILURE);
}
```
7. Mendapatkan value dari variabel key yang diambil dari function `get_system_key()` yang tertera pada header file. Apabila gagal untuk mendapatkan key-nya, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
	fprintf(stderr, "Error: Fail to create shared memory\n");
	exit(EXIT_FAILURE);
}
```
8. Function `shmget()` membuat segmen shared memory dengan key yang telah dibuat dan menyimpan data ID dari segmen shared memory tersebut ke dalam variabel `shmid`. Jika segmen shared memory dengan key tersebut belum ada, maka akan dibuat segmen shared memory yang baru menggunakan `IPC_CREAT` dengan izin read-write untuk semua user. Jika segmen sudah ada, maka function hanya perlu mengambil ID segmen shared memory yang sudah dibuat sebelumnya. Terakhir, apabila proses pembuatan segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
	fprintf(stderr, "Error: Fail to attach system data to shared memory\n");
	exit(EXIT_FAILURE);
}
```
9. Meng-attach segmen shared memory menggunakan function `shmat()` ke alamat memori yang dialokasikan ke program yang sedang berjalan sesuai dengan ID segmen shared memory yang telah diberikan. Apabila proses meng-attach segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke admin.

```c
if (sys->num_hunters < 0 || sys->num_hunters > MAX_HUNTERS) {
	sys->num_hunters = 0;
	sys->num_dungeons = 0;
	sys->current_notification_index = 0;
}
```
10. Jika program `system` dijalankan untuk pertama kalinya atau jumlah individual hunters yang terdaftar pada sistem melebihi batas limit, maka program akan menginisialisasi value untuk variabel jumlah hunters dan dungeons, serta indeks notifikasi untuk menyimpan data mengenai sistem kendali notifikasi setiap hunter ke value `0`.

```c
int opt;
```
11. Mendeklarasikan variabel, dimana:
- `opt`: untuk menyimpan data opsi yang dipilih oleh admin saat menjalankan program `hunter`.

```c
do {
    ...    
} while (opt != 6);
```
12. Merupakan suatu do-while loop untuk menampilkan UI pada layar terminal kepada admin selama admin tidak memilih opsi keenam yang mengeluarkan admin dari program.

```c
fprintf(stdout, "\e[H\e[2J\e[3J");
```
13. Membersihkan layar terminal. Memiliki peran yang sama layaknya command `clear`.

```c
fprintf(stdout, "=== SYSTEM MENU ===\n1. Hunter Info\n2. Dungeon Info\n3. Generate Dungeon\n4. Ban/Unban Hunter\n5. Reset Hunter\n6. Exit\nChoice: ");
```
14. Mengoutput UI ke layar terminal yang menampilkan pilihan opsi yang dapat diplih oleh admin.

```c
if (scanf("%d", &opt) != 1) {
    opt = -1;
    while (getchar() != '\n');
}
```
15. Mencoba untuk mendapatkan input dari admin dalam bentuk integer. Apabila admin menginput suatu karakter yang bukan merupakan suatu integer, maka opsi akan diinsialisasi dengan `-1` dan membersihkan buffer input dari karakter yang tidak sesuai tersebut.

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
16. Mengarahkan program ke function yang sesuai berdasarkan pada input yang diberikan oleh admin pada variabel `opt`. Jika value di dalam `opt` merupakan value yang bukan antara 1, 2, 3, 4, 5, atau 6 maka program akan melempar sebuah error ke stderr yang akan ditampilkan ke admin dan memberikan jeda tiga detik agar admin dapat membaca error tersebut.

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
17. Merupakan bagian dari subsoal 4.L: Destruksi Shared Memory.

```c
exit(EXIT_SUCCESS);
```
18. Jika admin memilih opsi keenam, maka do-while loop akan berhenti. Setelah itu, program dinyatakan berhasil dieksekusi dan keluar.

### • Soal  4.B: Registrasi dan Login Hunter

Pada subsoal 4.B: Registrasi dan Login Hunter, kita diperintahkan untuk membuat sebuah program untuk melakukan administrasi hunters yaitu proses registrasi dan login pada program `hunter`. Untuk membuat program ini dibuatlah tiga function bernama `regist_the_hunters()`, `login_registered_hunters()`, dan `login_menu_for_logged_in_hunters()`. Adapun penjelasan masing-masing fucntion adalah sebagai berikut:

#### a. Soal 4.B.1: `regist_the_hunters()`

Function `regist_the_hunters()` memiliki tugas utama yaitu melakukan proses pendaftaran atau registrasi untuk hunter baru . Adapun tampilan function `regist_the_hunters()` adalah sebagai berikut:

```c
void regist_the_hunters() {
    key_t key;
    int shmid;
    
    if ((key = get_system_key()) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory key\n");
        exit(EXIT_FAILURE);
    }

    if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory\n");
        exit(EXIT_FAILURE);
    }

    if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach to shared memory\n");
        exit(EXIT_FAILURE);
    }

    int numh = sys->num_hunters;
    if (numh > MAX_HUNTERS - 1) {
        fprintf(stdout, "Can't register. Hunter at full capacity\n");
        shmdt(sys);
        sleep(3);
        return;
    }
    
    
    char username[50];
    fprintf(stdout, "Username: ");
    scanf("%49s", username);
    
    for (int i = 0; i < numh; i++) {
        if (!strcmp(username, sys->hunters[i].username)) {
            fprintf(stderr, "Error: Username '%s' is already taken\n", username);
            shmdt(sys);
            sleep(3);
            exit(EXIT_FAILURE);
        }
    }
    
    struct Hunter *hunt = &sys->hunters[numh];
    strncpy(hunt->username, username, sizeof(hunt->username));
    hunt->level = 1; hunt->exp = 0; hunt->atk = 10; hunt->hp = 100; hunt->def = 5; hunt->banned = 0;

    key_t hkey;
    int hshmid;
    struct Hunter *huntshm;
    
    if ((hkey = ftok("/tmp/hunter", numh)) == -1) {
        fprintf(stderr, "Error: Fail to create hunter's key\n");
        shmdt(sys);
        exit(EXIT_FAILURE);
    }
    hunt->shm_key = hkey;
    
    if ((hshmid = shmget(hkey, sizeof(struct Hunter), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to create unique shared memory for each hunter\n");
        shmdt(sys);
        exit(EXIT_FAILURE);
    }
    
    if ((huntshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach hunter's shared memory\n");
        shmdt(sys);
        exit(EXIT_FAILURE);
    }
    
    memcpy(huntshm, hunt, sizeof(struct Hunter));
    
    shmdt(huntshm);

    sys->num_hunters++;

    shmdt(sys);
    fprintf(stdout, "Registration success!\n");
    sleep(3);
}
```

Dimana langkah implementasinya:

```c
void regist_the_hunters() {
	...
}
```
1. Mendeklarasikan function `regist_the_hunters()`.

```c
key_t key;
int shmid;
```
2. Mendeklarasikan variabel, dimana:
- `key`: untuk menyimpan data key utama yang digunakan untuk mengidentifikasi dan mengelola segmen shared memory mana yang akan digunakan bersama oleh program `hunter` dan `system`.
- `shmid`: untuk menyimpan data mengenai ID segmen shared memory yang digunakan oleh program `hunter` dan `system` untuk mengakses data.

```c
if ((key = get_system_key()) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory key\n");
        exit(EXIT_FAILURE);
}
```
3. Mendapatkan value dari variabel key yang diambil dari function `get_system_key()` yang tertera pada header file. Apabila gagal untuk mendapatkan key-nya, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory\n");
        exit(EXIT_FAILURE);
}
```
4. Function `shmget()` membuat segmen shared memory dengan key yang telah dibuat dan menyimpan data ID dari segmen shared memory tersebut ke dalam variabel `shmid`. Jika segmen shared memory dengan key tersebut belum ada, maka akan dibuat segmen shared memory yang baru menggunakan `IPC_CREAT` dengan izin read-write untuk semua user. Jika segmen sudah ada, maka function hanya perlu mengambil ID segmen shared memory yang sudah dibuat sebelumnya. Terakhir, apabila proses pembuatan segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach to shared memory\n");
        exit(EXIT_FAILURE);
}
```
5. Meng-attach segmen shared memory menggunakan function `shmat()` ke alamat memori yang dialokasikan ke program yang sedang berjalan sesuai dengan ID segmen shared memory yang telah diberikan. Apabila proses meng-attach segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
int numh = sys->num_hunters;
```
6. Mendeklarasikan variabel, dimana:
- `numh`: untuk menyimpan data banyaknya individual hunter yang ada pada sistem yang diambil dari mengakses data `num_hunters` yang disimpan pada segmen shared memory yang dikelola oleh program `system`.

```c
if (numh > MAX_HUNTERS - 1) {
	fprintf(stdout, "Can't register. Hunter at full capacity\n");
	shmdt(sys);
	sleep(3);
	return;
}
```
7. Memastikan bahwa banyak individual hunter saat hunter baru hendak mengregistrasi masih dibawah batas limit. Apabila banyaknya individual hunter sudah melebihi limit, maka program akan kembali ke halaman awal program `hunter` setelah melempar sebuah pesan ke stdout yang akan ditampilkan ke user, meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`, dan memberikan jeda tiga detik agar user dapat membaca pesan tersebut.

```c
char username[50];
```
8. Mendeklarasikan variabel, dimana:
- `username[50]`: untuk menyimpan data nama username hunter baru yang hendak registrasi dengan batas panjang 50 karakter.

```c
fprintf(stdout, "Username: ");
scanf("%49s", username);
```
9. Memerintahkan user untuk menginput nama username yang dikehendakinya untuk diregistrasi dan disimpan ke dalam sistem.

```c
for (int i = 0; i < numh; i++) {
	if (!strcmp(username, sys->hunters[i].username)) {
	    fprintf(stderr, "Error: Username '%s' is already taken\n", username);
	    shmdt(sys);
	    sleep(3);
	    exit(EXIT_FAILURE);
	}
}
```
10. Memastikan bahwa nama username yang dipilih saat registrasi belum pernah dipakai sebelumnya oleh hunter lain. Apabila username pernah dipakai, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user, meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`, dan memberikan jeda tiga detik agar user dapat membaca error tersebut.

```c
struct Hunter *hunt = &sys->hunters[numh];
strncpy(hunt->username, username, sizeof(hunt->username));
```
11. Jika username belum pernah dipakai sebelumnya, maka program akan membuat struktur Hunter lokal yang tidak terhubung ke segmen shared memory untuk hunter baru tersebut sesuai dengan urutan nomor hunter baru tersebut mendaftar dan menyimpan data usernamenya ke dalam struktur tersebut.

```c
hunt->level = 1; hunt->exp = 0; hunt->atk = 10; hunt->hp = 100; hunt->def = 5; hunt->banned = 0;
```
12. Menginisialisasi value status hunter baru ke value default yang telah ditentukan.

```c
key_t hkey;
int hshmid;
struct Hunter *huntshm;
```
13. Mendeklarasikan variabel, dimana:
- `hkey`: untuk menyimpan data key hunter baru yang hendak registrasi.
- `hshmid`: untuk menyimpan data mengenai ID segmen shared memory khusus yang digunakan untuk hunter baru yang hendak registrasi.
- `huntshm`: struktur Hunter yang terhubung pada segmen shared memory khusus untuk hunter baru yang hendak registrasi.

```c
if ((hkey = ftok("/tmp/hunter", numh)) == -1) {
        fprintf(stderr, "Error: Fail to create hunter's key\n");
        shmdt(sys);
        exit(EXIT_FAILURE);
}
```
14. Mendapatkan value dari variabel hkey yang diambil dari function `ftok()` terhadap pathname `/tmp/hunter`. Apabila gagal untuk mendapatkan hkey-nya, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`.

```c
hunt->shm_key = hkey;
```
15. Menginisialisasi value variabel `shm_key` dengan hkey yang didapat dari function `ftok()`.

```c
if ((hshmid = shmget(hkey, sizeof(struct Hunter), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to create unique shared memory for each hunter\n");
        shmdt(sys);
        exit(EXIT_FAILURE);
}
```
16. Function `shmget()` membuat segmen shared memory dengan key yang telah dibuat dan menyimpan data ID dari segmen shared memory tersebut ke dalam variabel `hshmid`. Jika segmen shared memory dengan key tersebut belum ada, maka akan dibuat segmen shared memory yang baru menggunakan `IPC_CREAT` dengan izin read-write untuk semua user. Jika segmen sudah ada, maka function hanya perlu mengambil ID segmen shared memory yang sudah dibuat sebelumnya. Terakhir, apabila proses pembuatan segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`.

```c
if ((huntshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach hunter's shared memory\n");
        shmdt(sys);
        exit(EXIT_FAILURE);
}
```
17. Meng-attach segmen shared memory menggunakan function `shmat()` ke alamat memori yang dialokasikan ke program yang sedang berjalan sesuai dengan ID segmen shared memory yang telah diberikan. Apabila proses meng-attach segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`.

```c
memcpy(huntshm, hunt, sizeof(struct Hunter));
```
18. Menyalin data yang ada pada struktur Hunter lokal ke struktur Hunter yang terhubung ke segmen shared memory khusus untuk hunter baru yang hendak registrasi.

```c
shmdt(huntshm);
```
19. Meng-detach segmen shared memory khusus untuk hunter yang hendak registrasi dengan menggunakan function `shmdt()`.

```c
sys->num_hunters++;
```
20. Menambah tally banyaknya individual hunter yang teregistrasi atau terdaftar di dalam sistem.

```c
shmdt(sys);
```
21. Meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`.

```c
fprintf(stdout, "Registration success!\n");
sleep(3);
```
22. Mengoutput suatu pesan ke user bahwa program registrasi berhasil berjalan dan user telah diregistrasi atau didaftarkan ke dalam sistem. Selain itu, user diberikan jeda tiga detik agar user tersebut dapat membaca output-nya.

#### b. Soal 4.B.2: `login_registered_hunters()`

Function `login_registered_hunters()` memiliki tugas utama yaitu melakukan proses login untuk hunter lama, khususnya melakukan proses verifikasi apakah user terdaftar di dalam sistem. Adapun tampilan function `login_registered_hunters()` adalah sebagai berikut:

```c
void login_registered_hunters() {
    key_t key;
    int shmid;
    
    if ((key = get_system_key()) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory key\n");
        exit(EXIT_FAILURE);
    }

    if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory\n");
        exit(EXIT_FAILURE);
    }

    if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach to shared memory\n");
        exit(EXIT_FAILURE);
    }

    char username[50];
    fprintf(stdout, "Username: ");
    scanf("%49s", username);
    while (getchar() != '\n');

    int hshmid;
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *hunt = &sys->hunters[i];
        if (!strcmp(username, hunt->username)) {
            if ((hshmid = shmget(hunt->shm_key, sizeof(struct Hunter), 0666)) == -1) {
                fprintf(stderr, "Error: Fail to get shared memory for the selected hunter\n");
                shmdt(sys);
                exit(EXIT_FAILURE);
            }
        
            struct Hunter *huntshm;
            if ((huntshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
                fprintf(stderr, "Error: Fail to attach hunter's shared memory\n");
                shmdt(sys);
                exit(EXIT_FAILURE);
            }


            login_menu_for_logged_in_hunters(huntshm, i);

            shmdt(huntshm);
            shmdt(sys);
            return;
        }
    }
    shmdt(sys);
    fprintf(stdout, "Can't login. Username not found\n");
    sleep(3);
    return;
}
```

Dimana langkah implementasinya:

```c
void login_registered_hunters() {
	...
}
```
1. Mendeklarasikan function `login_registered_hunters()`.

```c
key_t key;
int shmid;
```
2. Mendeklarasikan variabel, dimana:
- `key`: untuk menyimpan data key utama yang digunakan untuk mengidentifikasi dan mengelola segmen shared memory mana yang akan digunakan bersama oleh program `hunter` dan `system`.
- `shmid`: untuk menyimpan data mengenai ID segmen shared memory yang digunakan oleh program `hunter` dan `system` untuk mengakses data.

```c
if ((key = get_system_key()) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory key\n");
        exit(EXIT_FAILURE);
}
```
3. Mendapatkan value dari variabel key yang diambil dari function `get_system_key()` yang tertera pada header file. Apabila gagal untuk mendapatkan key-nya, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
if ((shmid = shmget(key, sizeof(struct SystemData), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory\n");
        exit(EXIT_FAILURE);
}
```
4. Function `shmget()` membuat segmen shared memory dengan key yang telah dibuat dan menyimpan data ID dari segmen shared memory tersebut ke dalam variabel `shmid`. Jika segmen shared memory dengan key tersebut belum ada, maka akan dibuat segmen shared memory yang baru menggunakan `IPC_CREAT` dengan izin read-write untuk semua user. Jika segmen sudah ada, maka function hanya perlu mengambil ID segmen shared memory yang sudah dibuat sebelumnya. Terakhir, apabila proses pembuatan segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
if ((sys = shmat(shmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach to shared memory\n");
        exit(EXIT_FAILURE);
}
```
5. Meng-attach segmen shared memory menggunakan function `shmat()` ke alamat memori yang dialokasikan ke program yang sedang berjalan sesuai dengan ID segmen shared memory yang telah diberikan. Apabila proses meng-attach segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user.

```c
char username[50];
```
6. Mendeklarasikan variabel, dimana:
- `username[50]`: untuk menyimpan data nama username hunter lama yang hendak login dengan batas panjang 50 karakter.

```c
fprintf(stdout, "Username: ");
scanf("%49s", username);
```
7. Memerintahkan user untuk menginput nama username yang dikehendakinya untuk dilakukan proses login.

```c
while (getchar() != '\n');
```
8. Jika input user melebihi batas panjang 50 karakter, maka sisa karakter yang ada pada buffer input akan dihapus.

```c
int hshmid;
```
9. Mendeklarasikan variabel, dimana:
- `hshmid`: untuk menyimpan data mengenai ID segmen shared memory khusus yang digunakan untuk hunter lama yang hendak login.

```c
for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *hunt = &sys->hunters[i];
        if (!strcmp(username, hunt->username)) {
		...
	}

	...
}
shmdt(sys);
fprintf(stdout, "Can't login. Username not found\n");
sleep(3);
return;
```
10. Memastikan bahwa nama username yang dipilih saat login terdaftar pada sistem. Apabila username tidak ditemukan, maka program akan kembali ke halaman awal program `hunter` setelah melempar sebuah pesan ke stdout yang akan ditampilkan ke user, meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`, dan memberikan jeda tiga detik agar user dapat membaca pesan tersebut.

```c
if ((hshmid = shmget(hunt->shm_key, sizeof(struct Hunter), 0666)) == -1) {
	fprintf(stderr, "Error: Fail to get shared memory for the selected hunter\n");
	shmdt(sys);
	exit(EXIT_FAILURE);
}
```
11. Function `shmget()` mengambil segmen shared memory dengan key yang telah disimpan pada variabel `shm_key` yang diambil dari struktur Hunter yang terhubung pada shared memory khusus untuk hunter lama yang hendak login dan menyimpan data ID dari segmen shared memory tersebut ke dalam variabel `hshmid`. Apabila proses pembuatan segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`.

```c
struct Hunter *huntshm;
```
12. Mendeklarasikan variabel, dimana:
- `huntshm`: struktur Hunter yang terhubung pada segmen shared memory khusus untuk hunter lama yang hendak login.

```c
if ((huntshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
	fprintf(stderr, "Error: Fail to attach hunter's shared memory\n");
	shmdt(sys);
	exit(EXIT_FAILURE);
}
```
13. Meng-attach segmen shared memory menggunakan function `shmat()` ke alamat memori yang dialokasikan ke program yang sedang berjalan sesuai dengan ID segmen shared memory yang telah diberikan. Apabila proses meng-attach segmen shared memory gagal, maka program akan keluar setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan meng-detach segmen shared memory utama dengan menggunakan function `shmdt()`.

```c
login_menu_for_logged_in_hunters(huntshm, i);
```
14. Masuk ke dalam function `login_menu_for_logged_in_hunters()` untuk melanjutkan proses login dengan menampilkan UI ke layar terminal dan mem-passing struktur Hunter yang terhubung pada segmen shared memory khusus untuk hunter lama yang hendak login dan urutan registrasi hunter dalam sistem.

```c
shmdt(huntshm);
shmdt(sys);
return;
```
15. Setelah function `login_menu_for_logged_in_hunters()` dijalankan dan user hendak kembali ke halaman awal, maka sebelumnya program akan meng-detach segmen shared memory utama dan segmen shared memory khusus untuk hunter lama yang hendak login dengan menggunakan function `shmdt()`.

#### c. Soal 4.B.3: `login_menu_for_logged_in_hunters()`

Function `login_menu_for_logged_in_hunters()` memiliki tugas utama yaitu melanjutkan proses login untuk hunter lama yang sudah terverifikasi oleh sistem melalui function `login_registered_hunters()` dan terbukti telah terdaftar sebelumnya. Adapun tampilan function `login_menu_for_logged_in_hunters()` adalah sebagai berikut:

```c
void login_menu_for_logged_in_hunters(struct Hunter *huntshm, int huntid) {
    int opt, tututitutut_toggle = 0;
    do {
        fprintf(stdout, "\e[H\e[2J\e[3J");
        fprintf(stdout, "=== HUNTER SYSTEM ===\n");

        if (tututitutut_toggle && sys->num_dungeons > 0) {
            if (sys->current_notification_index >= sys->num_dungeons) {
                sys->current_notification_index = 0;
            }
            struct Dungeon *davail = &sys->dungeons[sys->current_notification_index++];
            fprintf(stdout, "%s for minimum level %d opened!\n", davail->name, davail->min_level);
        }
        else {
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "===%s's MENU ===\n1. List Dungeon\n2. Raid\n3. Battle\n4. Toggle Notification\n5. Exit\nChoice: ", huntshm->username);

        if (scanf("%d", &opt) != 1) {
            opt = -1;
            while (getchar() != '\n');
        }
        switch (opt) {
        case 1:
            show_available_dungeons_based_on_hunters_level(huntshm->level);
            break;
        case 2:
            conquer_raidable_dungeons_based_on_hunters_level(huntshm, huntid);
            break;
        case 3:
            if (battle_other_hunters_based_on_stats(huntshm, huntid) == 1) {
                return;
            }
            break;
        case 4:
            tututitutut_toggle = tututitutut_toggle ? 0 : 1;
            fprintf(stdout, tututitutut_toggle ? "Notifications turned on!\n" : "Notifications turned off!\n");
            sleep(3);
            break;
        case 5:
            break;
        default:
            fprintf(stderr, "Error: Unknown Option\n");
            sleep(3);
        }
    } while (opt != 5);
    return;
}
```

Dimana langkah implementasinya:

```c
void login_menu_for_logged_in_hunters(struct Hunter *huntshm, int huntid) {
	...
}
```
1. Mendeklarasikan function `login_menu_for_logged_in_hunters()` dengan ketentuan:
- `struct Hunter *huntshm`: struktur Hunter yang terhubung pada segmen shared memory khusus untuk hunter lama yang hendak login.
- `int huntid`: urutan registrasi hunter dalam sistem.

```c
int opt, tututitutut_toggle = 0;
```
2. Mendeklarasikan variabel, dimana:
- `opt`: untuk menyimpan data opsi yang dipilih oleh user saat di dalam menu program `hunter`.
- `tututitutut_toggle`: untuk menyimpan data status sistem kendali notifikasi hunter apakah dinyalakan atau tidak.

```c
do {
    ...    
} while (opt != 5);
```
3. Merupakan suatu do-while loop untuk menampilkan UI pada layar terminal kepada user selama user tidak memilih opsi kelima yang mengembalikan user ke halaman awal program `hunter`.

```c
fprintf(stdout, "\e[H\e[2J\e[3J");
```
4. Membersihkan layar terminal. Memiliki peran yang sama layaknya command `clear`.

```c
fprintf(stdout, "=== HUNTER SYSTEM ===\n");
...
fprintf(stdout, "===%s's MENU ===\n1. List Dungeon\n2. Raid\n3. Battle\n4. Toggle Notification\n5. Exit\nChoice: ", huntshm->username);
```
5. Mengoutput UI ke layar terminal yang menampilkan pilihan opsi yang dapat diplih oleh user.

```c
if (tututitutut_toggle && sys->num_dungeons > 0) {
	if (sys->current_notification_index >= sys->num_dungeons) {
		sys->current_notification_index = 0;
	}
	struct Dungeon *davail = &sys->dungeons[sys->current_notification_index++];
	fprintf(stdout, "%s for minimum level %d opened!\n", davail->name, davail->min_level);
}
else {
	fprintf(stdout, "\n");
}
```
6.  Merupakan bagian dari subsoal 4.K: Tututitutut Toggle.

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
    show_available_dungeons_based_on_hunters_level(huntshm->level);
    break;
case 2:
    conquer_raidable_dungeons_based_on_hunters_level(huntshm, huntid);
    break;
case 3:
    if (battle_other_hunters_based_on_stats(huntshm, huntid) == 1) {
	return;
    }
    break;
case 4:
    tututitutut_toggle = tututitutut_toggle ? 0 : 1;
    fprintf(stdout, tututitutut_toggle ? "Notifications turned on!\n" : "Notifications turned off!\n");
    sleep(3);
    break;
case 5:
    break;
default:
    fprintf(stderr, "Error: Unknown Option\n");
    sleep(3);
}
```
8. Mengarahkan program ke function yang sesuai berdasarkan pada input yang diberikan oleh user pada variabel `opt`. Jika value di dalam `opt` merupakan value yang bukan antara 1, 2, 3, 4, atau 5 maka program akan melempar sebuah error ke stderr yang akan ditampilkan ke user dan memberikan jeda tiga detik agar user dapat membaca error tersebut. Khusus untuk opsi keempat, program tidak akan diarahkan ke function yang sesuai, namun hanya mengatur sistem kendali yang merupakan bagian dari subsoal 4.K: Tututitutut Toggle.

```c
return;
```
9. Jika user memilih opsi kelima, maka do-while loop akan berhenti. Setelah itu, function yang tertera pada function `login_menu_for_logged_in_hunters()` dinyatakan berhasil dieksekusi dan user diarahkan kembali ke halaman awal.

### • Soal  4.C: Informasi Hunter

Pada subsoal 4.C: Informasi Hunter, kita diperintahkan untuk membuat sebuah program untuk melihat daftar user yang teregistrasi dan tersimpan pada program `system`. Untuk membuat program ini dibuatlah suatu function bernama `info_of_all_hunters()`, dengan tampilan sebagai berikut:

```c
void info_of_all_hunters() {
    if (sys->num_hunters == 0) {
        fprintf(stderr, "\nNo hunters registered yet\n");
        sleep(3);
        return;
    }
    else {
        fprintf(stdout, "\n=== HUNTER INFO ===\n");
        for (int i = 0; i < sys->num_hunters; i++) {
            struct Hunter *hunt = &sys->hunters[i];
            fprintf(stdout, "Name: %-49s Level: %-3d EXP: %-3d ATK: %-3d HP: %-3d DEF: %-3d Banned: %-3s Key: %d\n",
                    hunt->username,
                    hunt->level,
                    hunt->exp,
                    hunt->atk,
                    hunt->hp,
                    hunt->def,
                    (hunt->banned ? "Yes" : "No"),
                    hunt->shm_key
                    );
        }
    }
    fprintf(stdout, "\nPress Enter key to return to menu");
    while (getchar()!='\n');
    getchar();
}
```

Dimana langkah implementasinya:

```c
void info_of_all_hunters() {
	...
}
```
1. Mendeklarasikan function `info_of_all_hunters()`.

```c
if (sys->num_hunters == 0) {
	fprintf(stderr, "\nNo hunters registered yet\n");
	sleep(3);
	return;
}
```
2. Memastikan bahwa setidaknya terdapat satu hunter yang terdaftar pada sistem. Apabila terindikasi bahwa tidak terdapat hunter yang teregistrasi, maka program akan kembali ke halaman awal setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan memberikan jeda tiga detik agar user dapat membaca error tersebut.

```c
else {
        fprintf(stdout, "\n=== HUNTER INFO ===\n");
	...
}
```
3. Menampilkan UI berupa header ke layar terminal.

```c
for (int i = 0; i < sys->num_hunters; i++) {
    struct Hunter *hunt = &sys->hunters[i];
    fprintf(stdout, "Name: %-49s Level: %-3d EXP: %-3d ATK: %-3d HP: %-3d DEF: %-3d Banned: %-3s Key: %d\n",
	    hunt->username,
	    hunt->level,
	    hunt->exp,
	    hunt->atk,
	    hunt->hp,
	    hunt->def,
	    (hunt->banned ? "Yes" : "No"),
	    hunt->shm_key
	    );
}
```
4. Merupakan suatu for-loop yang mengiterasi terhadap setiap hunter yang tersimpan pada struktur Hunter yang terhubung pada segmen shared memory utama dan menampilkan username dan status setiap hunter tersebut ke layar terminal.

```c
fprintf(stdout, "\nPress Enter key to return to menu");
while (getchar()!='\n');
getchar();
```
5. Merupakan suatu fitur untuk menunggu user menekan Enter key agar program `system` kembali ke halaman awal. Digunakan agar user dapat membaca tampilan informasi hunter tanpa perlu mempertimbangkan constraint waktu yang diterapkan oleh function `sleep()`.

### • Soal  4.D: Generasi Dungeon

Pada subsoal 4.D: Generasi Dungeon, kita diperintahkan untuk membuat sebuah program pada `system` untuk meng-generate suatu dungeon yang memiliki nama tersendiri, value status random di antara rentang tertentu, dan level minimum yang diperlukan oleh hunters untuk menaklukkan dungeon tersebut. Untuk membuat program ini dibuatlah suatu function bernama `generate_random_dungeons()`, dengan tampilan sebagai berikut:

```c
void generate_random_dungeons() {
    int numd = sys->num_dungeons;
    if (numd > MAX_DUNGEONS - 1) {
        fprintf(stderr, "Can't generate. Dungeons at full capacity\n");
        sleep(3);
        return;
    }


    struct Dungeon *dngn = &sys->dungeons[numd];

    char *possinames[] = {
        "Double Dungeon",
        "Demon Castle",
        "Pyramid Dungeon",
        "Red Gate Dungeon",
        "Hunters Guild Dungeon",
        "Busan A-Rank Dungeon",
        "Insects Dungeon",
        "Goblins Dungeon",
        "D-Rank Dungeon",
        "Gwanak Mountain Dungeon",
        "Hapjeong Subway Station Dungeon"
    };
    size_t possicounts = sizeof(possinames)/sizeof(possinames[0]);
    int randdngnid = rand() % possicounts;
    snprintf(dngn->name, sizeof(dngn->name), "%s", possinames[randdngnid]);
    dngn->min_level = rand() % 5 + 1; dngn->atk = rand() % 51 + 100; dngn->hp = rand() % 51 + 50; dngn->def = rand() %26 + 25; dngn->exp = rand() % 151 + 150;

    key_t dkey;
    int dshmid;
    struct Dungeon *dngnshm;

    if ((dkey = ftok("/tmp/dungeon", numd)) == -1) {
        fprintf(stderr, "Error: Fail to create dungeon's key\n");
        exit(EXIT_FAILURE);
    }
    dngn->shm_key = dkey;
    
    if ((dshmid = shmget(dkey, sizeof(struct Dungeon), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Error: Fail to create unique shared memory for each dungeon\n");
        exit(EXIT_FAILURE);
    }
    
    if ((dngnshm = shmat(dshmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach dungeon's shared memory\n");
        exit(EXIT_FAILURE);
    }
    
    memcpy(dngnshm, dngn, sizeof(struct Dungeon));
    
    shmdt(dngnshm);

    sys->num_dungeons++;

    fprintf(stdout, "Dungeon generated!\n");
    fprintf(stdout, "Name: %-49s\nMinimum Level: %-3d\nEXP Reward: %-3d\nATK: %-3d\nHP: %-3d\nDEF: %-3d\n",
            dngn->name,
            dngn->min_level,
            dngn->exp,
            dngn->atk,
            dngn->hp,
            dngn->def);

    fprintf(stdout, "\nPress Enter key to return to menu");
    while (getchar()!='\n');
    getchar();
}
```

## REVISI

## soal 3

dungeon.c
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define MAX_BUFFER 1024

struct Weapon {
    int id;
    char name[50];
    int price;
    int damage;
    char passive[50];
};

struct Player {
    int id;
    char name[50];
    int gold;
    char weapon[50];
    int damage;
    char passive[50];
    int kills;
    struct Weapon inventory[10];
    int inventory_size;
    struct Weapon equipped_weapon;
};

struct Player players[10];
struct Weapon weapons[5];
int enemy_health = 200;

// Import dari shop.c
extern const char* buy_weapon(int player_id, int weapon_id);

void init_shop() {
    weapons[0] = (struct Weapon){1, "Sea Halberd", 75, 15, ""};
    weapons[1] = (struct Weapon){2, "War Axe", 70, 20, ""};
    weapons[2] = (struct Weapon){3, "Windtalker", 75, 20, "10% Attack Speed"};
    weapons[3] = (struct Weapon){4, "Blade of Despair", 100, 70, "+16 Physical Attack"};
}

void init_player(int id, const char* name) {
    players[id].id = id;
    strcpy(players[id].name, name);
    players[id].gold = 100;
    strcpy(players[id].weapon, "Fists");
    players[id].damage = 5;
    players[id].inventory_size = 0;
    strcpy(players[id].passive, "");
    players[id].kills = 0;
    players[id].equipped_weapon = (struct Weapon){0};
}

void send_inventory(int client_sock, int player_id) {
    char response[MAX_BUFFER] = "INVENTORY:";
    for (int i = 0; i < players[player_id].inventory_size; i++) {
        char weapon_info[100];
        sprintf(weapon_info, "%d. %s (Dmg: %d)", i+1, 
                players[player_id].inventory[i].name, 
                players[player_id].inventory[i].damage);
        
        if (strlen(players[player_id].inventory[i].passive) > 0) {
            sprintf(weapon_info + strlen(weapon_info), " | Passive: %s", 
                    players[player_id].inventory[i].passive);
        }
        strcat(response, weapon_info);
        strcat(response, ";");
    }
    send(client_sock, response, strlen(response), 0);
}

void equip_weapon(int client_sock, int player_id, int weapon_index) {
    if (weapon_index < 1 || weapon_index > players[player_id].inventory_size) {
        send(client_sock, "Invalid weapon index!", 21, 0);
        return;
    }
    
    players[player_id].equipped_weapon = players[player_id].inventory[weapon_index-1];
    strcpy(players[player_id].weapon, players[player_id].equipped_weapon.name);
    players[player_id].damage = players[player_id].equipped_weapon.damage;
    strcpy(players[player_id].passive, players[player_id].equipped_weapon.passive);

    if (strstr(players[player_id].passive, "+16 Physical Attack")) {
        players[player_id].damage += 16;
    }

    send(client_sock, "Weapon equipped!", 16, 0);
}

void battle(int player_id, int client_sock) {
    char response[MAX_BUFFER] = {0};
    int enemy_hp = 100 + rand() % 101;
    int damage = players[player_id].damage + (rand() % 10);

    if (strcmp(players[player_id].equipped_weapon.name, "Blade of Despair") == 0) {
        strcat(response, "Passive Activated: +16 Physical Attack!\n");
        damage += 16;
    }

    if (strcmp(players[player_id].equipped_weapon.name, "Windtalker") == 0 && (rand() % 10 == 0)) {
        strcat(response, "Passive Activated: 10% Attack Speed!\n");
    }

    if (rand() % 3 == 0) {
        damage *= 2;
        strcat(response, "=== CRITICAL HIT! ===\n");
    }

    enemy_hp -= damage;

    char damage_msg[50];
    sprintf(damage_msg, "You dealt %d damage!\n", damage);
    strcat(response, damage_msg);

    if (enemy_hp <= 0) {
        int reward = 20 + rand() % 31;
        players[player_id].gold += reward;
        players[player_id].kills++;

        char reward_msg[50];
        sprintf(reward_msg, "Enemy defeated! Reward: %d gold.\n", reward);
        strcat(response, reward_msg);
    } else {
        char health_msg[50];
        sprintf(health_msg, "Enemy Health: %d/200\n", enemy_hp);
        strcat(response, health_msg);
    }

    send(client_sock, response, strlen(response), 0);
}

void handle_command(int client_sock, int player_id) {
    char buffer[MAX_BUFFER] = {0};
    read(client_sock, buffer, MAX_BUFFER);

    if (strncmp(buffer, "STATS", 5) == 0) {
        char response[MAX_BUFFER];
        sprintf(response, "Gold: %d | Weapon: %s | Damage: %d | Kills: %d | Passive: %s",
                players[player_id].gold, players[player_id].weapon, 
                players[player_id].damage, players[player_id].kills, 
                players[player_id].passive);
        send(client_sock, response, strlen(response), 0);
    } 
    else if (strncmp(buffer, "BUY ", 4) == 0) {
        int weapon_id = atoi(buffer + 4);
        const char* result = buy_weapon(player_id, weapon_id);
        send(client_sock, result, strlen(result), 0);
    }
    else if (strncmp(buffer, "BATTLE", 6) == 0) {
        battle(player_id, client_sock);
    }
    else if (strncmp(buffer, "INVENTORY", 9) == 0) {
        send_inventory(client_sock, player_id);
    }
    else if (strncmp(buffer, "EQUIP ", 6) == 0) {
        int weapon_index = atoi(buffer + 6);
        equip_weapon(client_sock, player_id, weapon_index);
    }
    else {
        const char* msg = "Unknown command!";
        send(client_sock, msg, strlen(msg), 0);
    }
}

int main() {
    srand(time(NULL));
    init_shop();
    init_player(0, "Hero");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(sockfd, 5);
    printf("Server started on port 8080...\n");

    int client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &addr_size);
    while (1) {
        handle_command(client_sock, 0);
    }

    close(client_sock);
    close(sockfd);
    return 0;
}


```

player.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER];

    // Membuat socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Menu interaktif
    while (1) {
        printf("\n==== MENU ====\n");
        printf("1. Show Stats\n2. Buy Weapon\n3. View Inventory\n4. Battle\n5. Exit\nChoice: ");   
        
        int choice;
        scanf("%d", &choice);
        getchar(); // consume newline
        memset(buffer, 0, MAX_BUFFER);

        switch (choice) {
            case 1:
                send(sock, "STATS", strlen("STATS"), 0);
                read(sock, buffer, MAX_BUFFER);
                printf("\n%s\n", buffer);
                break;

            case 2:
                printf("\nWeapons:\n1. Sea Halberd (75)\n2. War Axe (70)\n3. Windtalker (75)\n4. Blade of Despair (100)\n");
                printf("Choose weapon ID: ");
                int weapon_id;
                scanf("%d", &weapon_id);
                getchar(); // consume newline
                sprintf(buffer, "BUY %d", weapon_id);
                send(sock, buffer, strlen(buffer), 0);
                memset(buffer, 0, MAX_BUFFER);
                read(sock, buffer, MAX_BUFFER);
                printf("\n%s\n", buffer);
                break;

            case 3:
                send(sock, "INVENTORY", strlen("INVENTORY"), 0);
                memset(buffer, 0, MAX_BUFFER);
                read(sock, buffer, MAX_BUFFER);

                printf("\n=== INVENTORY ===\n");
                char *weapon = strtok(buffer, ";");
                int item_num = 0;
                while (weapon != NULL) {
                    if (strncmp(weapon, "INVENTORY:", 10) == 0) {
                        weapon += 10;
                    }
                    printf("%s\n", weapon);
                    item_num++;
                    weapon = strtok(NULL, ";");
                }

                printf("\nEnter weapon number to equip (0 to cancel): ");
                int choice_equip;
                scanf("%d", &choice_equip);
                getchar(); // consume newline
                if (choice_equip > 0 && choice_equip <= item_num) {
                    sprintf(buffer, "EQUIP %d", choice_equip);
                    send(sock, buffer, strlen(buffer), 0);
                    memset(buffer, 0, MAX_BUFFER);
                    read(sock, buffer, MAX_BUFFER);
                    printf("\n%s\n", buffer);
                }
                break;

            case 4:
                send(sock, "BATTLE", strlen("BATTLE"), 0);
                memset(buffer, 0, MAX_BUFFER);
                read(sock, buffer, MAX_BUFFER);
                printf("\n%s\n", buffer);
                break;

            case 5:
                printf("Exiting...\n");
                close(sock);
                return 0;

            default:
                printf("Invalid choice!\n");
        }

        memset(buffer, 0, MAX_BUFFER); // bersihkan buffer setiap akhir loop
    }

    return 0;
}
```
shop.c

```c
#include <string.h>
#include <stdio.h>

/
struct Weapon {
    int id;
    char name[50];
    int price;
    int damage;
    char passive[50];
};

struct Player {
    int id;
    char name[50];
    int gold;
    char weapon[50];
    int damage;
    char passive[50];
    struct Weapon inventory[10];
    int inventory_size;
    struct Weapon equipped_weapon;
};

// Deklarasi variabel global dari dungeon.c
extern struct Player players[10];
extern struct Weapon weapons[5];

// Fungsi utama pembelian senjata
const char* buy_weapon(int player_id, int weapon_id) {
    struct Weapon w = weapons[weapon_id - 1];

    if (players[player_id].gold >= w.price) {
        players[player_id].gold -= w.price;

        // Masukkan ke inventory
        players[player_id].inventory[players[player_id].inventory_size++] = w;

        // Auto-equip jika belum punya
        if (strcmp(players[player_id].weapon, "Fists") == 0) {
            strcpy(players[player_id].weapon, w.name);
            players[player_id].damage = w.damage;
            strcpy(players[player_id].passive, w.passive);
            players[player_id].equipped_weapon = w;
        }

        return "Purchase successful!";
    } else {
        return "Not enough gold!";
    }
}

```


