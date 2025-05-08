# Laporan Resmi Modul 3 Kelompok IT-07
## Anggota

| Nama 				| NRP		|
|-------------------------------|---------------|
| Muhammad Rakha Hananditya R.	| 5027241015 	|
| Zaenal Mustofa		| 5027241018 	|
| Mochkamad Maulana Syafaat	| 5027241021 	|

## • Soal  1
### “The Legend of Rootkids” 
Soal ini kita ditugaskan untuk membuat sistem RPC server-client untuk membantu memproses file teks tersebut menjadi gambar JPEG sesuai dengan instruksi yang diberikan. Sistem ini akan melibatkan dua bagian:
1. Server yang menerima permintaan dari client untuk mengubah teks menjadi gambar, serta mengirimkan gambar yang telah diproses.
2. Client yang mengirimkan perintah untuk mendekripsi teks dan mendownload gambar yang telah dihasilkan.
### Server (image_server.c)
Server ini akan berjalan sebagai daemon yang mendengarkan permintaan dari client melalui socket, mengelola dekripsi file teks menjadi gambar JPEG, dan mengirimkan gambar jika diminta.
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// Fungsi untuk membalik string
void reverse(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; ++i) {
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
    }
}

// Fungsi untuk mengubah hex menjadi byte
void hex_to_bin(const char *hex, unsigned char *out, size_t *out_len) {
    size_t len = strlen(hex);
    *out_len = 0;
    for (size_t i = 0; i < len; i += 2) {
        sscanf(hex + i, "%2hhx", &out[*out_len]);
        (*out_len)++;
    }
}

// Fungsi untuk menangani permintaan upload
void handle_upload(int client_socket) {
    char buffer[1024];
    int bytes_received;
    FILE *output_file;
    
    // Terima nama file
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0';
    
    // Terima data file
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0';
    
    // Reverse string
    reverse(buffer);
    
    // Decode Hex to binary
    unsigned char decoded_data[1024];
    size_t decoded_len;
    hex_to_bin(buffer, decoded_data, &decoded_len);
    
    // Simpan gambar dengan timestamp sebagai nama file
    time_t t;
    struct tm *tm_info;
    char filename[64];
    time(&t);
    tm_info = localtime(&t);
    strftime(filename, sizeof(filename), "%s.jpeg", tm_info);
    
    output_file = fopen(filename, "wb");
    fwrite(decoded_data, 1, decoded_len, output_file);
    fclose(output_file);
    
    // Kirim konfirmasi kepada client
    send(client_socket, "File uploaded successfully", 26, 0);
}

// Fungsi untuk menangani permintaan download
void handle_download(int client_socket) {
    char filename[64];
    FILE *input_file;
    size_t file_size;
    char *file_buffer;
    
    // Terima nama file
    recv(client_socket, filename, sizeof(filename), 0);
    
    input_file = fopen(filename, "rb");
    if (input_file == NULL) {
        send(client_socket, "File not found", 15, 0);
        return;
    }
    
    // Tentukan ukuran file
    fseek(input_file, 0, SEEK_END);
    file_size = ftell(input_file);
    rewind(input_file);
    
    // Kirim file ke client
    file_buffer = (char *)malloc(file_size);
    fread(file_buffer, 1, file_size, input_file);
    send(client_socket, file_buffer, file_size, 0);
    
    free(file_buffer);
    fclose(input_file);
}

// Main server
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // Membuat socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket failed");
        exit(1);
    }
    
    // Menyiapkan alamat server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Server listening on port 12345...\n");
    
    // Menunggu client untuk connect
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        char command[64];
        recv(client_socket, command, sizeof(command), 0);
        
        if (strcmp(command, "upload") == 0) {
            handle_upload(client_socket);
        } else if (strcmp(command, "download") == 0) {
            handle_download(client_socket);
        }
        
        close(client_socket);
    }
    
    close(server_socket);
    return 0;
}
```
### Client (image_server.c)
Client ini akan mengirimkan file teks untuk didekripsi oleh server, serta meminta gambar yang telah didekripsi dari server.
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Fungsi untuk mengirim file ke server
void upload_file(int server_socket, const char *filename) {
    FILE *file = fopen(filename, "r");
    char buffer[1024];
    int bytes_read;
    
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }
    
    // Kirim perintah upload
    send(server_socket, "upload", 6, 0);
    
    // Kirim nama file
    send(server_socket, filename, strlen(filename), 0);
    
    // Kirim data file
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(server_socket, buffer, bytes_read, 0);
    }
    
    fclose(file);
    
    // Terima konfirmasi dari server
    recv(server_socket, buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);
}

// Fungsi untuk mengunduh file dari server
void download_file(int server_socket, const char *filename) {
    FILE *file = fopen(filename, "wb");
    char buffer[1024];
    int bytes_received;
    
    // Kirim perintah download
    send(server_socket, "download", 8, 0);
    
    // Kirim nama file
    send(server_socket, filename, strlen(filename), 0);
    
    // Terima file dari server
    while ((bytes_received = recv(server_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }
    
    fclose(file);
    printf("File downloaded successfully: %s\n", filename);
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    
    // Membuat socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket failed");
        exit(1);
    }
    
    // Menyiapkan alamat server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Alamat IP server
    server_addr.sin_port = htons(12345);
    
    // Connect ke server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        exit(1);
    }
    
    // Upload file
    upload_file(server_socket, "secrets/input_1.txt");
    
    // Download file
    download_file(server_socket, "1744403652.jpeg");
    
    close(server_socket);
    return 0;
}
```

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
### delivery_agent
Program ini bertugas melakukan pengiriman otomatis untuk pesanan bertipe Express menggunakan multi-threading. Program ini juga dapat membaca file.csv dan menyimpannya ke shared memory. Berikut ini penjelasan kode nya :
```c
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

```
Bagian ini mendefinisikan tipe data enumerasi dan struktur untuk merepresentasikan informasi pesanan dalam sistem. Tipe enum OrderType digunakan untuk membedakan jenis pesanan, yaitu EXPRESS untuk pengiriman cepat dan REGULER untuk pengiriman biasa, sedangkan OrderStatus menunjukkan status pengiriman dengan nilai PENDING atau DELIVERED. Struct Order menyimpan data lengkap untuk satu pesanan, seperti nama penerima, alamat tujuan, tipe dan status pesanan, serta nama agen pengantar. Semua pesanan disimpan dalam struct SharedData yang memuat array orders dan variabel count sebagai penghitung jumlah pesanan yang aktif. Pointer global SharedData *data digunakan untuk mengakses shared memory yang berisi seluruh data pesanan, sehingga dapat diakses bersama oleh beberapa thread atau program.
```c
void write_log(const char *agent, const char *name, const char *address) {
    FILE *log = fopen("delivery.log", "a");
    ...
    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] Express package delivered to %s in %s\n", ...)
    ...
    fclose(log);
}
```
Membuka file log delivery.log dalam mode append, mencatat informasi waktu, nama agen, penerima, dan alamat, lalu menutup file.
```c
void *agent_thread(void *arg) {
    char *agent = (char *)arg;
    while (1) {
        for (int i = 0; i < data->count; ++i) {
            if (data->orders[i].type == EXPRESS && data->orders[i].status == PENDING) {
                ...
            }
        }
        sleep(2);
    }
    return NULL;
}
```
Fungsi `agent_thread` bertugas menjalankan proses pengantaran untuk pesanan dengan tipe `EXPRESS` secara terus-menerus dalam thread yang berbeda. Argumen yang diterima adalah nama agen, seperti `"AGENT A"`, yang kemudian digunakan untuk menandai pesanan yang berhasil diantar. Di dalam perulangan `while(1)`, thread akan memindai seluruh array pesanan yang ada di shared memory. Jika ditemukan pesanan dengan tipe `EXPRESS` dan status masih `PENDING`, maka status pesanan akan diubah menjadi `DELIVERED`, nama agen akan dicatat dalam entri pesanan, dan informasi pengiriman akan ditulis ke dalam file log menggunakan fungsi `write_log`. Setelah mengantar satu pesanan, thread melakukan `sleep(1)` sebagai simulasi waktu pengiriman sebelum melanjutkan pemeriksaan berikutnya. Setelah seluruh pesanan discan, thread akan tidur selama dua detik (`sleep(2)`) sebelum mengulangi proses pemindaian, memberikan jeda agar tidak membebani CPU.
```c
int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | IPC_EXCL | 0666);
data = (SharedData *)shmat(shmid, NULL, 0);
FILE *file = fopen("delivery_order.csv", "r");
while (fgets(line, sizeof(line), file)) {
    char *name = strtok(line, ",");
    char *address = strtok(NULL, ",");
    char *type = strtok(NULL, "\n");

    if (...) {
        Order *o = &data->orders[data->count++];
        strncpy(...);
        o->type = (strcmp(type, "Express") == 0) ? EXPRESS : REGULER;
        o->status = PENDING;
        strcpy(o->agent, "-");
    }
}
else {
    shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    data = (SharedData *)shmat(shmid, NULL, 0);
}

```
Pada bagian ini, program akan membuka file `delivery_order.csv` yang berisi daftar pesanan yang perlu dimuat ke dalam shared memory. File dibuka dalam mode baca (`"r"`), dan setiap baris dibaca satu per satu menggunakan `fgets`. Untuk setiap baris, data akan dipecah berdasarkan tanda koma menggunakan fungsi `strtok`, menghasilkan tiga bagian utama yaitu `name` (nama penerima), `address` (alamat tujuan), dan `type` (jenis pengiriman). Apabila ketiganya valid dan jumlah pesanan belum melebihi batas maksimum (`MAX_ORDERS`), maka program akan mengisi elemen berikutnya dalam array `orders` yang ada di shared memory. Nama, alamat, dan tipe order disalin ke dalam struktur `Order`, kemudian status pesanan diatur sebagai `PENDING`, menandakan bahwa pesanan belum dikirim, dan nama agen diisi dengan tanda `"-"` sebagai placeholder sebelum ditugaskan agen pengantar. Apabila file tidak bisa dibuka karena shared memory sudah diinisialisasi sebelumnya oleh proses lain, maka program akan langsung mengakses shared memory yang telah ada menggunakan `shmget` dan `shmat`. Bagian ini memastikan bahwa semua proses dispatcher dan agen memiliki akses konsisten ke data pesanan yang sama.
```c
pthread_t t1, t2, t3;
pthread_create(&t1, NULL, agent_thread, "AGENT A");
pthread_create(&t2, NULL, agent_thread, "AGENT B");
pthread_create(&t3, NULL, agent_thread, "AGENT C");

pthread_join(t1, NULL);
pthread_join(t2, NULL);
pthread_join(t3, NULL);
```
Pada bagian ini, program membuat tiga thread menggunakan `pthread_create`, yang masing-masing dijalankan sebagai agen pengantar dengan nama "AGENT A", "AGENT B", dan "AGENT C". Ketiga agen ini akan bekerja secara paralel untuk memantau shared memory dan mengeksekusi pengiriman pesanan bertipe *Express* secara otomatis. Fungsi `agent_thread` akan dijalankan oleh setiap thread dan terus menerus mengecek apakah ada pesanan bertipe *Express* yang masih berstatus *PENDING*. Jika ditemukan, pesanan tersebut akan ditandai sebagai *DELIVERED*, nama agen dicatat, dan pengiriman akan dilog-kan ke dalam file `delivery.log`. Setelah semua thread dibuat, fungsi `pthread_join` digunakan untuk menunggu setiap thread menyelesaikan tugasnya. Karena setiap agen berjalan dalam loop tak hingga, `pthread_join` ini juga memastikan agar proses utama tidak langsung keluar dan tetap menunggu agen-agen tersebut aktif selama program berjalan.

### dispatcher
Program `dispatcher.c` bertugas untuk menangani pesanan bertipe *Reguler* yang tidak dikirim otomatis oleh thread seperti pada *delivery\_agent.c*. Program ini menerima argumen dari baris perintah dan menyediakan tiga opsi: `-deliver [Nama]` untuk mengirim paket reguler secara manual, `-status [Nama]` untuk melihat status pengiriman sebuah pesanan, dan `-list` untuk menampilkan semua pesanan beserta statusnya. Di awal, program mencoba mengakses shared memory menggunakan `shmget` dan `shmat` agar dapat membaca dan memodifikasi data pesanan yang telah di-load oleh `delivery_agent`.

Saat perintah `-deliver` diberikan, program mencari nama pelanggan pada data shared memory dan memastikan bahwa pesanan tersebut merupakan pesanan reguler yang masih berstatus *PENDING*. Jika ditemukan, status akan diubah menjadi *DELIVERED* dan nama agen pengantar diambil dari variabel lingkungan `USER` atau `LOGNAME` untuk mencatat siapa yang mengeksekusi pengiriman tersebut. Selanjutnya, informasi pengiriman ditulis ke dalam file `delivery.log` melalui fungsi `write_log`.

Jika perintah yang diberikan adalah `-status`, maka program akan menampilkan status pesanan pelanggan tertentu, apakah sudah terkirim beserta nama agen, atau masih menunggu. Sementara itu, perintah `-list` digunakan untuk menampilkan daftar semua pesanan beserta statusnya satu per satu. Dengan pendekatan ini, `dispatcher.c` memberikan kontrol manual kepada pengguna untuk menangani pengiriman reguler secara fleksibel, sambil tetap terintegrasi dengan sistem shared memory dan logging yang digunakan oleh *delivery\_agent.c*.

Berikut ini penjelasan kodenya :
```c
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
```
Fungsi `write_log` bertugas untuk menulis log pengiriman pesanan ke file delivery.log. Fungsi ini menggunakan `time(NULL)` dan `localtime()` untuk mendapatkan waktu saat ini dan menulis log ke dalam file dengan format yang sudah ditentukan, mencatat waktu, agen, dan informasi pesanan (nama dan alamat).
```c
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n  ./dispatcher -deliver [Nama]\n  ./dispatcher -status [Nama]\n  ./dispatcher -list\n");
        return 1;
    }
```
Pada awal fungsi main, program memeriksa jumlah argumen yang diberikan. Jika jumlah argumen kurang dari 2, maka akan menampilkan pesan penggunaan program dan keluar.
```c
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("Shared memory not found");
        return 1;
    }
    SharedData *data = (SharedData *)shmat(shmid, NULL, 0);
```
Di sini, program mencoba mengakses shared memory menggunakan `shmget` dengan kunci `SHM_KEY`. Jika shared memory tidak ditemukan, program menampilkan pesan kesalahan dan keluar. Jika ditemukan, program akan mengaitkan shared memory tersebut dengan pointer `data` menggunakan `shmat`.
```c
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
```
Jika perintah yang diberikan adalah `-deliver`, program akan mencari pesanan berdasarkan nama yang diberikan `(argv[2])`. Jika pesanan ditemukan dan berstatus `PENDING`, maka status pesanan akan diubah menjadi `DELIVERED`, dan nama agen yang mengirimkan pesanan akan diambil dari variabel lingkungan `USER` atau `LOGNAME`. Setelah itu, informasi pengiriman dicatat di file log dengan memanggil fungsi `write_log`.
```c
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
```
Jika perintah yang diberikan adalah `-status`, program akan mencari pesanan berdasarkan nama yang diberikan. Jika pesanan ditemukan, statusnya akan ditampilkan. Jika sudah dikirim, nama agen pengantar juga akan ditampilkan.
```c
    } else if (strcmp(argv[1], "-list") == 0) {
        for (int i = 0; i < data->count; i++) {
            printf("%s - %s\n", data->orders[i].name,
                   data->orders[i].status == DELIVERED ? "Delivered" : "Pending");
        }
```
Jika perintah yang diberikan adalah `-list`, program akan menampilkan seluruh pesanan yang ada beserta statusnya (apakah sudah dikirim atau masih pending).
```c
    } else {
        printf("Invalid command.\n");
    }
```
Jika perintah yang diberikan tidak sesuai dengan yang dikenali (misalnya bukan `-deliver`, `-status`, atau `-list`), maka program akan menampilkan pesan bahwa perintah tidak valid.
## • Soal  3

# The Lost Dungeon - Sistem Adventure Berbasis RPC

Sebuah dungeon adventure multiplayer dengan fitur eksplorasi, battle, dan manajemen inventory menggunakan arsitektur **RPC server-client**.

## 🏗️ Struktur Program

soal_3/
├── dungeon.c # Server utama
├── player.c # Client pemain
└── shop.c # Sistem toko senjata


## 🖥️ Komponen Utama

### **Server (`dungeon.c`)**
- Berjalan sebagai **daemon process** di background
- Mendukung koneksi dari **multiple clients** secara paralel
- Fungsi utama:
  - Memproses perintah RPC dari client
  - Mengelola state dungeon dan battle
  - Menyimpan data senjata dari `shop.c`

### **Client (`player.c`)**
- Terhubung ke server via **socket RPC**
- Menu interaktif dengan fitur:
  - 🏰 Eksplorasi dungeon
  - ⚔️ Battle mode
  - 🛒 Toko senjata
  - 🎒 Inventory management
  - 📊 Status player

## ⚙️ Fitur Wajib

### 🏪 Toko Senjata (`shop.c`)
| Requirement           | Detail                                                                 |
|-----------------------|-----------------------------------------------------------------------|
| Jumlah senjata        | Minimal 5 jenis                                                      |
| Spesifikasi           | - Harga<br>- Damage value<br>- Minimal 2 senjata dengan passive effect |
| Contoh Passive        | Critical chance +15%, Lifesteal 10% HP                               |
| Inventory System      | Penyimpanan senjata yang dibeli                                      |

### ⚔️ Battle System
Musuh random: HP (50-200), rewards (gold/EXP random).
Damage calculation:
Base damage + rumus random (misal: rand() % 20 + 10).
Critical hit (2x damage) dengan chance tertentu.
Health bar visual yang dinamis.
Efek passive senjata saat battle (contoh: "Passive activated: Lifesteal!").

## penyelesaian
langkah pertama membuat file server
```
dungeon.c
```
Header dan  Konstanta dan Struktur Data
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "shop.c"

#define PORT 8080
#define MAX_BUFFER 1024


typedef struct {
    int id;
    char name[50];
    int price;
    int damage;
    char passive[50];
} Weapon;

typedef struct {
    int gold;
    int damage;
    char weapon[50];
    char passive[50];
    int kills;
    Weapon inventory[10]; 
    int inventory_size;    
    Weapon equipped_weapon; 
} Player;

Player players[10]; 
int enemy_health; 

extern void init_shop();
extern const char* buy_weapon(int player_id, int weapon_id);
```

Fungsi Utama
send_inventory(client_sock, player_id)
Mengirim daftar senjata dalam inventori pemain ke client melalui socket.

equip_weapon(client_sock, player_id, weapon_index)
Mengequip senjata dari inventori dan menerapkan efek pasif jika ada.

battle(player_id)
Simulasi pertarungan:
Musuh punya HP 100–200
Damage pemain ditentukan oleh senjata dan bonus acak
Efek pasif dan critical hit dapat terjadi
Jika musuh kalah, pemain mendapat gold dan jumlah kill bertambah

handle_command(client_sock, player_id)
Menerima perintah dari client dan memanggil fungsi terkait:
STATS → Kirim statistik pemain
BUY <id> → Beli senjata dari shop
BATTLE → Lawan musuh
INVENTORY → Tampilkan inventori
EQUIP <index> → Equips senjata dari inventori

```c

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
    

    if (strstr(players[player_id].equipped_weapon.passive, "+16 Physical Attack")) {
        players[player_id].equipped_weapon.damage += 16;
    }
    
    send(client_sock, "Weapon equipped!", 16, 0);
}

void battle(int player_id) {
    char response[MAX_BUFFER] = {0};
    int enemy_hp = 100 + rand() % 101;
    int damage = players[player_id].damage + (rand() % 10);

    // Cek passive Blade of Despair
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

  
    enemy_health -= damage;


    char damage_msg[50];
    sprintf(damage_msg, "You dealt %d damage!\n", damage);
    strcat(response, damage_msg);

    if (enemy_health <= 0) {
        int reward = 20 + rand() % 31;
        players[player_id].gold += reward;
        players[player_id].kills++;
        
        char reward_msg[50];
        sprintf(reward_msg, "Enemy defeated! Reward: %d gold.\n", reward);
        strcat(response, reward_msg);
    } else {
        char health_msg[50];
        sprintf(health_msg, "Enemy Health: %d/200\n", enemy_health);
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
        battle(player_id);
        send(client_sock, "Enemy defeated! Check stats.", 28, 0);
    }
    else if (strncmp(buffer, "INVENTORY", 9) == 0) {
    send_inventory(client_sock, player_id);
    }
    else if (strncmp(buffer, "EQUIP ", 6) == 0) {
    int weapon_index = atoi(buffer + 6);
    equip_weapon(client_sock, player_id, weapon_index);
    }
}
```

 Fungsi main()
Inisialisasi shop dan atribut dasar pemain.
Membuat socket TCP, bind ke PORT, dan listen.
Menangani koneksi client satu per satu.
Menjalankan handle_command() saat client terhubung.
```c
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    srand(time(NULL));

    // Inisialisasi shop dan player
    init_shop();
    for (int i = 0; i < 10; i++) {
        players[i].gold = 500;
        strcpy(players[i].weapon, "Fists");
        players[i].damage = 10;
        strcpy(players[i].passive, "None");
    }

    // Setup socket server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("=== Dungeon Server Running (Port: %d) ===\n", PORT);

    // Terima koneksi client
    int current_player = 0;
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        printf("Player %d connected\n", current_player);
        handle_command(new_socket, current_player);
        current_player++;
        close(new_socket);
    }

    return 0;
}
```
Membuat file yang berisi weapon yang akan dibeli
```
shop.c
```
Struktur `Weapon`
```c
#include <string.h>

typedef struct {
    int id;
    char name[50];
    int price;
    int damage;
    char passive[50];
} Weapon;

Weapon weapons[5];
```

Fungsi init_shop()
Inisialisasi senjata yang tersedia di toko.
```c
void init_shop() {
    weapons[0] = (Weapon){1, "Sea Halberd", 75, 15, ""};
    weapons[1] = (Weapon){2, "War Axe", 70, 20, ""};
    weapons[2] = (Weapon){3, "Windtalker", 75, 20, "10% Attack Speed"};
    weapons[3] = (Weapon){4, "Blade of Despair", 100, 70, "+16 Physical Attack"};
}
```
 Fungsi buy_weapon()
 Digunakan untuk membeli senjata berdasarkan ID.
Mengecek apakah pemain memiliki cukup gold.
```c
const char* buy_weapon(int player_id, int weapon_id) {
    extern Player players[10];   // Referensi dari dungeon.c
    Weapon w = weapons[weapon_id - 1];

    if (players[player_id].gold >= w.price) {
        players[player_id].gold -= w.price;
        strcpy(players[player_id].weapon, w.name);
        players[player_id].damage = w.damage;
        strcpy(players[player_id].passive, w.passive);
        return "Purchase successful!";
    } else {
        return "Not enough gold!";
    }
}
```
Terakhir membuat file player yang bisa terkoneksi dengan server
```
player.c
```
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
    char buffer[MAX_BUFFER] = {0};


  Connect ke server
  ```c
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }
```

 Menu interaktif
```c
    while (1) {
        printf("\n==== MENU ====\n");
        printf("1. Show Stats\n2. Buy Weapon\n3. View Inventory\n4. Battle\n5. Exit\nChoice: ");   
        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                send(sock, "STATS", 5, 0);
                read(sock, buffer, MAX_BUFFER);
                printf("\n%s\n", buffer);
                break;
            case 2:
                printf("Weapons:\n1. Sea Halberd (75)\n2. War Axe (70)\n3. Windtalker (75)\n4. Blade of Despair (100)\n");
                printf("Choose weapon ID: ");
                int weapon_id;
                scanf("%d", &weapon_id);
                sprintf(buffer, "BUY %d", weapon_id);
                send(sock, buffer, strlen(buffer), 0);
                read(sock, buffer, MAX_BUFFER);
                printf("\n%s\n", buffer);
                break;
                case 3:
                send(sock, "INVENTORY", 9, 0);
                read(sock, buffer, MAX_BUFFER);
                printf("\n=== INVENTORY ===\n");
                char *weapon = strtok(buffer, ";");
                while (weapon != NULL) {
                    if (strstr(weapon, "INVENTORY:") == weapon) {
                        weapon += 10; // Skip header
                    }
                    printf("%s\n", weapon);
                    weapon = strtok(NULL, ";");
                }
                printf("\nEnter weapon number to equip (0 to cancel): ");
                int choice_equip;
                scanf("%d", &choice_equip);
                if (choice_equip != 0) {
                    sprintf(buffer, "EQUIP %d", choice_equip);
                    send(sock, buffer, strlen(buffer), 0);
                    read(sock, buffer, MAX_BUFFER);
                    printf("\n%s\n", buffer);
                }
                break;
            case 4:
                send(sock, "BATTLE", 6, 0);
                read(sock, buffer, MAX_BUFFER);
                printf("\n%s\n", buffer);
                break;
            case 5:
                close(sock);
                return 0;
            default:
                printf("Invalid choice!\n");
        }
    }

    return 0;
}

```

### KENDALA

sulit untuk mengkoneksikan file jika tidak menggunakan makefile .h

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

### • Soal  4.E: Informasi Dungeon

Pada subsoal 4.C: Informasi Dungeon, kita diperintahkan untuk membuat sebuah program untuk melihat daftar dungeon yang telah di-generate dan tersimpan pada program `system`. Untuk membuat program ini dibuatlah suatu function bernama `info_of_all_dungeons()`, dengan tampilan sebagai berikut:

```c
void info_of_all_dungeons() {
    if (sys->num_dungeons == 0) {
        fprintf(stderr, "\nNo dungeons have yet to be generated\n");
        sleep(3);
        return;
    }
    else {
        fprintf(stdout, "\n=== DUNGEON INFO ===\n");
        for (int i = 0; i < sys->num_dungeons; i++) {
            struct Dungeon *dngn = &sys->dungeons[i];
            fprintf(stdout, "[Dungeon %d]\n", i + 1);
            fprintf(stdout, "Name: %-49s\nMinimum Level: %-3d\nEXP Reward: %-3d\nATK: %-3d\nHP: %-3d\nDEF: %-3d\nKey: %d\n\n",
                    dngn->name,
                    dngn->min_level,
                    dngn->exp,
                    dngn->atk,
                    dngn->hp,
                    dngn->def,
                    dngn->shm_key);
        }
    }
    fprintf(stdout, "\nPress Enter key to return to menu");
    while (getchar()!='\n');
    getchar();
}
```


Dimana langkah implementasinya:

```c
void info_of_all_dungeons() {
	...
}
```
1. Mendeklarasikan function `info_of_all_dungeons()`.

```c
if (sys->num_dungeons == 0) {
        fprintf(stderr, "\nNo dungeons have yet to be generated\n");
        sleep(3);
        return;
}
```
2. Memastikan bahwa setidaknya terdapat satu dungeon yang telah di-generate oleh sistem. Apabila terindikasi bahwa tidak terdapat dungeon yang ter-generate, maka program akan kembali ke halaman awal setelah melempar sebuah error ke stderr yang akan ditampilkan ke user dan memberikan jeda tiga detik agar user dapat membaca error tersebut.

```c
else {
        fprintf(stdout, "\n=== DUNGEON INFO ===\n");
	...
}
```
3. Menampilkan UI berupa header ke layar terminal.

```c
for (int i = 0; i < sys->num_dungeons; i++) {
    struct Dungeon *dngn = &sys->dungeons[i];
    fprintf(stdout, "[Dungeon %d]\n", i + 1);
    fprintf(stdout, "Name: %-49s\nMinimum Level: %-3d\nEXP Reward: %-3d\nATK: %-3d\nHP: %-3d\nDEF: %-3d\nKey: %d\n\n",
	    dngn->name,
	    dngn->min_level,
	    dngn->exp,
	    dngn->atk,
	    dngn->hp,
	    dngn->def,
	    dngn->shm_key);
}
```
4. Merupakan suatu for-loop yang mengiterasi terhadap setiap dungeon yang tersimpan pada struktur Dungeon yang terhubung pada segmen shared memory utama dan menampilkan nama dan status setiap dungeon tersebut ke layar terminal.

```c
fprintf(stdout, "\nPress Enter key to return to menu");
while (getchar()!='\n');
getchar();
```
5. Merupakan suatu fitur untuk menunggu user menekan Enter key agar program `system` kembali ke halaman awal. Digunakan agar user dapat membaca tampilan informasi dungeon tanpa perlu mempertimbangkan constraint waktu yang diterapkan oleh function `sleep()`.

### • Soal  4.F: Appropiate Dungeon

Pada subsoal 4.F: Appropiate Dungeon, kita diperintahkan untuk membuat sebuah program untuk melihat daftar dungeon yang sesuai dengan level hunter dan dapat dlakukan proses raid pada program `hunter`. Untuk membuat program ini dibuatlah suatu function bernama `show_available_dungeons_based_on_hunters_level()`, dengan tampilan sebagai berikut:

```c
void show_available_dungeons_based_on_hunters_level(int huntlvl) {
    int dshmid;
    int lsid = 0;
    fprintf(stdout, "\n=== AVAILABLE DUNGEONS ===\n");
    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon *dngn = &sys->dungeons[i];
        if ((dshmid = shmget(dngn->shm_key, sizeof(struct Dungeon), 0666)) == -1) {
            fprintf(stderr, "Error: Fail to get shared memory for the list of dungeons\n");
            sleep(3);
            return;
        }
        
        struct Dungeon *dngnshm;
        if ((dngnshm = shmat(dshmid, NULL, 0)) == (void *)-1) {
            fprintf(stderr, "Error: Fail to attach dungeon's shared memory\n");
            sleep(3);
            return;
        }

        if (dngnshm->min_level <= huntlvl) {
            fprintf(stdout, "%2d. %-49s (Level %d+) %d\n", ++lsid, dngnshm->name, dngnshm->min_level, huntlvl);
        }

        shmdt(dngnshm);
    }
    if (!lsid) {
        fprintf(stdout, "No dungeons available for level %d\n", huntlvl);
    }
    fprintf(stdout, "\nPress Enter key to return to menu");
    while (getchar()!='\n');
    getchar();
}
```

### • Soal  4.G: Dungeon Raid

Pada subsoal 4.G: Dungeon Raid, kita diperintahkan untuk membuat sebuah program untuk melakukan raid dengan memilih sebuah dungeon dari daftar dungeon yang memiliki level sesuai dengan level hunter. Untuk membuat program ini dibuatlah suatu function bernama `conquer_raidable_dungeons_based_on_hunters_level()`, dengan tampilan sebagai berikut:

```c
void conquer_raidable_dungeons_based_on_hunters_level(struct Hunter *huntshm, int huntid) {
    if (huntshm->banned) {
        fprintf(stderr, "\nError: You are banned from engaging in battles or raids\n");
        sleep(3);
        return;
    }
    int dshmid;
    int lsid = 0;
    int raidable[MAX_DUNGEONS];
    fprintf(stdout, "\n=== RAIDABLE DUNGEONS ===\n");
    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon *dngn = &sys->dungeons[i];
        if ((dshmid = shmget(dngn->shm_key, sizeof(struct Dungeon), 0666)) == -1) {
            fprintf(stderr, "Error: Fail to get shared memory for the list of dungeons\n");
            sleep(3);
            return;
        }
        
        struct Dungeon *dngnshm;
        if ((dngnshm = shmat(dshmid, NULL, 0)) == (void *)-1) {
            fprintf(stderr, "Error: Fail to attach dungeon's shared memory\n");
            sleep(3);
            return;
        }

        if (dngnshm->min_level <= huntshm->level) {
            raidable[lsid] = i;
            fprintf(stdout, "%2d. %-49s (Level %d+)\n", ++lsid, dngnshm->name, dngnshm->min_level);
        }

        shmdt(dngnshm);
    }
    if (!lsid) {
        fprintf(stdout, "No dungeons available for level %d\n", huntshm->level);
        fprintf(stdout, "\nPress Enter key to return to menu");
        while (getchar()!='\n');
        getchar();
        return;
    }
    else {
        int opt;
        fprintf(stdout, "Choose Dungeon: ");
        if (scanf("%d", &opt) != 1 || opt < 1 || opt > lsid) {
            while (getchar() != '\n');
            fprintf(stderr, "Error: Unknown Option\n");
            sleep(3);
            return;
        }

        struct Dungeon *targ = &sys->dungeons[raidable[opt - 1]];
        if ((dshmid = shmget(targ->shm_key, sizeof(struct Dungeon), 0666)) == -1) {
            fprintf(stderr, "Error: Fail to get shared memory for target dungeon\n");
            sleep(3);
            return;
        }
        
        struct Dungeon *dngnshm;
        if ((dngnshm = shmat(dshmid, NULL, 0)) == (void *)-1) {
            fprintf(stderr, "Error: Fail to attach dungeon's shared memory\n");
            sleep(3);
            return;
        }

        fprintf(stdout, "Raid success! Gained:\nATK: %-3d\nHP: %-3d\nDEF: %-3d\nEXP: %-3d\n", dngnshm->atk, dngnshm->hp, dngnshm->def, dngnshm->exp);
        huntshm->atk += dngnshm->atk;
        huntshm->def += dngnshm->def;
        huntshm->hp += dngnshm->hp;
        huntshm->exp += dngnshm->exp;

        if (huntshm->exp >= 500) {
            huntshm->exp -= 500;
            huntshm->level++;
            fprintf(stdout, "%s leveled up!\n", huntshm->username);
        }

        memcpy(&sys->hunters[huntid], huntshm, sizeof(struct Hunter));

        shmctl(targ->shm_key, IPC_RMID, NULL);
        sys->dungeons[raidable[opt - 1]] = sys->dungeons[--sys->num_dungeons];
        shmdt(dngnshm);

        fprintf(stdout, "\nPress Enter key to return to menu");
        while (getchar()!='\n');
        getchar();
    }
}
```

### • Soal  4.H: Hunters Waging War

Pada subsoal 4.H: Hunters Waging War, kita diperintahkan untuk membuat sebuah program untuk melakukan pertarungan antara satu hunter dengan yang lain dengan kemenangan berdasarkan kekuatan dari masing-masing hunter. Untuk membuat program ini dibuatlah suatu function bernama `battle_other_hunters_based_on_stats()`, dengan tampilan sebagai berikut:

```c
int battle_other_hunters_based_on_stats(struct Hunter *huntshm, int huntid) {
    if (huntshm->banned) {
        fprintf(stderr, "\nError: You are banned from engaging in battles or raids\n");
        sleep(3);
        return 0;
    }
    if (sys->num_hunters < 2) {
        fprintf(stdout, "Can't battle. No other hunters to battle\n");
        sleep(3);
        return 0;
    }
    int hshmid;
    fprintf(stdout, "\n=== PVP LIST ===\n");
    for (int i = 0; i < sys->num_hunters; i++) {
        if (i == huntid) {
            continue;
        }
        else {
            struct Hunter *foe = &sys->hunters[i];
            if ((hshmid = shmget(foe->shm_key, sizeof(struct Hunter), 0666)) == -1) {
                fprintf(stderr, "Error: Fail to get shared memory for the list of hunters\n");
                sleep(3);
                return 1;
            }
            
            struct Hunter *foeshm;
            if ((foeshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
                fprintf(stderr, "Error: Fail to attach list of foe's shared memory\n");
                sleep(3);
                return 1;
            }
            int foestat = foeshm->atk + foeshm->def + foeshm->hp;
            fprintf(stdout, "%s - Total Power: %d\n", foeshm->username, foestat);

            shmdt(foeshm);
        }
    }
    char targ[50];
    fprintf(stdout, "Target: ");
    scanf("%49s", targ);
    while (getchar() != '\n');

    int targid = -1;
    for (int i = 0; i < sys->num_hunters; i++) {
        if (i == huntid) {
            continue;
        }
        if (!strcmp(sys->hunters[i].username, targ)) {
            targid = i;
            break;
        }
    }

    if (targid == -1) {
        fprintf(stdout, "Battle not initiated. No hunter with that username\n");
        sleep(3);
        return 0;
    }
    
    struct Hunter *targg = &sys->hunters[targid];
    if ((hshmid = shmget(targg->shm_key, sizeof(struct Hunter), 0666)) == -1) {
        fprintf(stderr, "Error: Fail to get shared memory for the targeted hunter\n");
        sleep(3);
        return 0;
    }
    
    struct Hunter *targgshm;
    if ((targgshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
        fprintf(stderr, "Error: Fail to attach list of target's shared memory\n");
        sleep(3);
        return 0;
    }

    int huntstat = huntshm->atk + huntshm->def + huntshm->hp;
    int targgstat = targgshm->atk + targgshm->def + targgshm->hp;
    fprintf(stdout, "You choose to battle %s\n", targgshm->username);
    fprintf(stdout, "Your Power: %d\nOpponent's Power: %d\n", huntstat, targgstat);

    if (huntstat > targgstat) {
        fprintf(stdout, "Deleting defender's shared memory (shmid: %d)\n", targgshm->shm_key);
        fprintf(stdout, "Battle won! You acquired %s's stats\n", targgshm->username);

        huntshm->atk += targgshm->atk;
        huntshm->def += targgshm->def;
        huntshm->hp += targgshm->hp;
        huntshm->exp += targgshm->exp;

        if (huntshm->exp >= 500) {
            huntshm->exp -= 500;
            huntshm->level++;
            fprintf(stdout, "%s leveled up!\n", huntshm->username);
        }
        

        memcpy(&sys->hunters[huntid], huntshm, sizeof(struct Hunter));

        shmctl(targgshm->shm_key, IPC_RMID, NULL);
        sys->hunters[targid] = sys->hunters[--sys->num_hunters];
        shmdt(targgshm);
        
    }
    else {
        fprintf(stdout, "Deleting YOUR shared memory (shmid: %d)\n", huntshm->shm_key);
        fprintf(stdout, "Battle lost! %s acquired your stats\n", targgshm->username);

        targgshm->atk += huntshm->atk;
        targgshm->def += huntshm->def;
        targgshm->hp += huntshm->hp;
        targgshm->exp += huntshm->exp;

        if (targgshm->exp >= 500) {
            targgshm->exp -= 500;
            targgshm->level++;
            fprintf(stdout, "%s leveled up!\n", targgshm->username);
        }
        

        memcpy(&sys->hunters[targid], targgshm, sizeof(struct Hunter));

        shmctl(huntshm->shm_key, IPC_RMID, NULL);
        sys->hunters[huntid] = sys->hunters[--sys->num_hunters];
        shmdt(huntshm);
        shmdt(targgshm);
        sleep(5);
        return 1;
    }

    fprintf(stdout, "\nPress Enter key to return to menu");
    while (getchar()!='\n');
    getchar();
    return 0;
}
```

### • Soal  4.I: Hunters Gatekeeping

Pada subsoal 4.I: Hunters Gatekeeping, kita diperintahkan untuk membuat sebuah program untuk dapat melakukan pemblokiran ataupun mengangkat blokir tersebut terhadap suatu hunter. Untuk membuat program ini dibuatlah suatu function bernama `ban_hunter_because_hunter_bad()`, dengan tampilan sebagai berikut:

```c
void ban_hunter_because_hunter_bad() {
    char username[50];
    fprintf(stdout, "Target: ");
    scanf("%49s", username);
    while (getchar() != '\n');

    int hshmid;
    int exist = 0;
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *hunt = &sys->hunters[i];
        if (!strcmp(username, hunt->username)) {
            exist = 1;

            if ((hshmid = shmget(hunt->shm_key, sizeof(struct Hunter), 0666)) == -1) {
                fprintf(stderr, "Error: Fail to get shared memory for the selected hunter\n");
                exit(EXIT_FAILURE);
            }
            
            struct Hunter *huntshm;
            if ((huntshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
                fprintf(stderr, "Error: Fail to attach hunter's shared memory\n");
                exit(EXIT_FAILURE);
            }

            if (huntshm->banned) {
                char decision;
                fprintf(stdout, "Hunter %s is already banned. Unban?\n(Y/n): ", username);
                decision = getchar();
                while (getchar() != '\n');

                if (decision == 'Y' || decision == 'y') {
                    huntshm->banned = 0;
                    memcpy(hunt, huntshm, sizeof(struct Hunter));
                    fprintf(stdout, "Hunter %s has been unbanned\n", username);
                }
                else {
                    fprintf(stdout, "Hunter %s is already banned. Exiting ban/unban sequence\n", username);
                    shmdt(huntshm);
                    sleep(3);
                    return;
                }
            }
            else {
                huntshm->banned = 1;
                memcpy(hunt, huntshm, sizeof(struct Hunter));
                fprintf(stdout, "Hunter %s has been banned\n", username);
            }


            shmdt(huntshm);

            sleep(3);
            return;
        }
    }
    if (!exist) {
        fprintf(stderr, "No one is banned/unbanned. No hunter with that username\n");
        sleep(3);
        return;
    }
}
```

### • Soal  4.J: Reset Hunters

Pada subsoal 4.J: Reset Hunters, kita diperintahkan untuk membuat sebuah program untuk dapat melakukan reset status suatu hunter ke value default yang telah ditentukan oleh program. Untuk membuat program ini dibuatlah suatu function bernama `reset_the_hunters_stat_back_to_default()`, dengan tampilan sebagai berikut:

```c
void reset_the_hunters_stat_back_to_default() {
    char username[50];
    fprintf(stdout, "Target: ");
    scanf("%49s", username);
    while (getchar() != '\n');

    int hshmid;
    int exist = 0;
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *hunt = &sys->hunters[i];
        if (!strcmp(username, hunt->username)) {
            exist = 1;
            if ((hshmid = shmget(hunt->shm_key, sizeof(struct Hunter), 0666)) == -1) {
                fprintf(stderr, "Error: Fail to get shared memory for the selected hunter\n");
                exit(EXIT_FAILURE);
            }
            

            struct Hunter *huntshm;
            if ((huntshm = shmat(hshmid, NULL, 0)) == (void *)-1) {
                fprintf(stderr, "Error: Fail to attach hunter's shared memory\n");
                exit(EXIT_FAILURE);
            }

            huntshm->level = 1;
            huntshm->exp = 0;
            huntshm->atk = 10;
            huntshm->hp = 100;
            huntshm->def = 5;
            huntshm->banned = 0;

            memcpy(hunt, huntshm, sizeof(struct Hunter));

            shmdt(huntshm);

            fprintf(stdout, "Hunter %s stats has been resetted to defaults\n", username);
            sleep(3);
            return;
        }
    }
    if (!exist) {
        fprintf(stderr, "Nothing resetted. No hunter with that username\n");
        sleep(3);
        return;
    }
}
```

### • Soal  4.K: Tututitutut Toggle

Pada subsoal 4.K: Tututitutut Toggle, kita diperintahkan untuk membuat sebuah program untuk para hunter dapat mengaktifkan dan menonaktifkan sistem kendali notifikasi terhadap terbukanya suatu dungeon baik sesuai dengan level hunter ataupun tidak. Untuk membuat program ini diperbaruilah function bernama `login_menu_for_logged_in_hunters()` untuk dapat mengakomodasi program tersebut, dengan tampilan sebagai berikut:

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

Beserta penambahan opsi berikut pada switch-case du function yang sama:

```c
case 4:
    tututitutut_toggle = tututitutut_toggle ? 0 : 1;
    fprintf(stdout, tututitutut_toggle ? "Notifications turned on!\n" : "Notifications turned off!\n");
    sleep(3);
    break;
```

### • Soal  4.L: Destruksi Shared Memory

Pada subsoal 4.L: Destruksi Shared Memory, kita diperintahkan untuk membuat sebuah program untuk menghapus semua data yang tersimpan pada segmen shared memory saat hendak keluar dari program `system`. Untuk membuat program ini diperbaruilah function `main()` pada program `system` untuk dapat mengakomodasi program  4.L: Destruksi Shared Memory tersebut, dengan tampilan sebagai berikut:

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

### • Soal 4: Kendala yang Dialami

<p align="center">
	<img src="https://github.com/user-attachments/assets/f3bd0b51-6376-4074-8d76-ac3388eadc22" alt="It sure does feel lonely in these area." width="640" height="360">  
</p>

> (1) Screenshot potret tampilan program `system` dan `hunter` saat dijalankan dan terjadi logic error.

Pada kasus ini dapat terlihat bahwa meskipun hanya terdapat satu user yang terdaftar pada sistem saat program mengeksekusi function `battle_other_hunters_based_on_stats()` dan menginput username yang menjadi target dimana tidak ada target dengan username tersebut, program akan memilih satu-satunya username yang terdaftar pada sistem, alias user yang sedang login pada program `hunter` sebagai target. Alhasil, terjadilah proses self-annihilation dimana program akan menyatakan bahwa user kalah terhadap dirinya sendiri dan program akan keluar secara tiba-tiba setelah itu karena gagal mengambil data user karena user sudah dihapus oleh sistem.

### • Soal 4: Dokumentasi Lain yang Tidak Berkaitan dengan Kendala

<p align="center">
	<img src="https://github.com/user-attachments/assets/9a4373aa-50d8-49be-9bd7-233178e64a84" alt="Does anybody actually has the time to read these?" width="640" height="360">  
</p>

<p align="center">
	<img src="https://github.com/user-attachments/assets/046e0c26-dfa9-495b-94f9-8f0412cfd398" alt="I do believe that one day someone will see this. Or maybe not? Who knows." width="640" height="360">  
</p>

<p align="center">
	<img src="https://github.com/user-attachments/assets/b96d29fb-a5ad-4155-9c40-d53bc18a0cd5" alt="Thinking back, maybe I should've focused more on actually writing the documentation." width="640" height="360">  
</p>

## • REVISI

### • Soal 3

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


![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161358.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161418.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161436.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161453.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161529.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161552.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161629.png)
![alt text](https://github.com/jagosyafaat30/dokumetnsasi/blob/main/sisop3/Screenshot%202025-05-08%20161644.png)

### • Soal 4

Pada soal 4 terdapat revisi di mana sistem notifikasi pada awalnya masih bersifat statis dan belum bisa memperbarui dirinya sendiri setiap tiga detik. Adapun bagian program `hunter` yang mengalami perubahan agar masalah tersebut dapat diselesaikan adalah sebagai berikut:

```c
int is_user_in_login_menu = 0;
int is_tututitutut_on = 0;
int is_tututitutut_toggle_function_on;
// Merupakan variabel-variabel yang bersifat global

void *tututitutut_toggle(void *arg);

void login_menu_for_logged_in_hunters(struct Hunter *huntshm, int huntid) {
    is_tututitutut_toggle_function_on = 1;

    pthread_t tid;
    pthread_create(&tid, NULL, tututitutut_toggle, NULL);

    int opt;
    do {
        is_user_in_login_menu = 1;
        fprintf(stdout, "\e[H\e[2J\e[3J");
        fprintf(stdout, "=== HUNTER SYSTEM ===\n");
        fprintf(stdout, "===%s's MENU ===\n1. List Dungeon\n2. Raid\n3. Battle\n4. Toggle Notification\n5. Exit\nChoice: ", huntshm->username);

        if (scanf("%d", &opt) != 1) {
            opt = -1;
            while (getchar() != '\n');
        }
        is_user_in_login_menu = 0;

        switch (opt) {
        case 1:
            show_available_dungeons_based_on_hunters_level(huntshm->level);
            break;
        case 2:
            conquer_raidable_dungeons_based_on_hunters_level(huntshm, huntid);
            break;
        case 3:
            if (battle_other_hunters_based_on_stats(huntshm, huntid) == 1) {
                opt = 5;
            }
            break;
        case 4:
            is_tututitutut_on = is_tututitutut_on ? 0 : 1;
            fprintf(stdout, is_tututitutut_on ? "Notifications turned on!\n" : "Notifications turned off!\n");
            sleep(3);
            break;
        case 5:
            break;
        default:
            fprintf(stderr, "Error: Unknown Option\n");
            sleep(3);
        }
    } while (opt != 5);

    is_tututitutut_toggle_function_on = 0;
    pthread_join(tid, NULL);
    return;
}

void *tututitutut_toggle(void *arg) {
    (void)arg;
    int notif_cycle = 0;
    while (is_tututitutut_toggle_function_on) {
        sleep(3);
        if (!is_tututitutut_on || !is_user_in_login_menu || sys->num_dungeons == 0) {
            continue;
        }
        if (notif_cycle >= sys->num_dungeons) {
            notif_cycle = 0;
        }
        struct Dungeon *davail = &sys->dungeons[notif_cycle++];
        fprintf(stdout, "\033[1;1H%s for minimum level %d opened!\n", davail->name, davail->min_level);
    }
    return NULL;
}
```

#### • Kendala yang masih dialami

Pada program tersebut baris yang berisi notifikasi diletakkan pada baris pertama pada layar terminal. Alhasil, baris yang berisi `=== HUNTER SYSTEM ===` secara terpaksa terhapus dari layar terminal dan letak untuk menginput opsi pada terminal berpindah ke baris kedua yang menyebabkan karakter yang diinputkan tidak begitu terlihat pada layar terminal. Apabila pada program diimplementasikan fitur untuk mencegah hal tersebut terjadi, misalnya dengan menggunakan `\n` atau sejenisnya, maka baris notifikasi tidak akan muncul sama sekali. Begitu pula percobaan untuk mengembalikan kursor ke posisi setelah kata `Choice: `akan menyebabkan baris notifikasi tidak muncul. Namun, secara keseluruhan sistem notifikasi sudah dapat memperbarui dirinya sendiri setiap tiga detik.
