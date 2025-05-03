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