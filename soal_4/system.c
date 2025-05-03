#include "shm_common.h"

void info_of_all_hunters();
void info_of_all_dungeons();
void generate_random_dungeons();
void reset_the_hunters_stat_back_to_default();
void ban_hunter_because_hunter_bad();

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