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

    // Connect ke server
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

    // Menu interaktif
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