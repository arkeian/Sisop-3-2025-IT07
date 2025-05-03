#include "shm_common.h"

struct SystemData *sys;

void login_menu_for_logged_in_hunters(struct Hunter *huntshm, int huntid);
void regist_the_hunters();
void login_registered_hunters();
void show_available_dungeons_based_on_hunters_level(int huntlvl);
void conquer_raidable_dungeons_based_on_hunters_level(struct Hunter *huntshm, int huntid);
int battle_other_hunters_based_on_stats(struct Hunter *huntshm, int huntid);


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