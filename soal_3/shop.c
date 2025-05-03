#include <string.h>

typedef struct {
    int id;
    char name[50];
    int price;
    int damage;
    char passive[50];
} Weapon;

Weapon weapons[5];

void init_shop() {
    weapons[0] = (Weapon){1, "Sea Halberd", 75, 15, ""};
    weapons[1] = (Weapon){2, "War Axe", 70, 20, ""};
    weapons[2] = (Weapon){3, "Windtalker", 75, 20, "10% Attack Speed"};
    weapons[3] = (Weapon){4, "Blade of Despair", 100, 70, "+16 Physical Attack"};
}

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