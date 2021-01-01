#pragma once

#include <glib.h>

enum NVCOMPUTER_STATE
{
    CS_UNKNOWN,
    CS_ONLINE,
    CS_OFFLINE
};
typedef enum NVCOMPUTER_STATE NVCOMPUTER_STATE;

enum NVCOMPUTER_PAIR_STATE
{
    PS_UNKNOWN,
    PS_PAIRED,
    PS_NOT_PAIRED
};
typedef enum NVCOMPUTER_PAIR_STATE NVCOMPUTER_PAIR_STATE;

struct NVCOMPUTER_T
{
    // Ephemeral traits
    NVCOMPUTER_STATE state;
    NVCOMPUTER_PAIR_STATE pair_state;
    char active_address[40];
    int current_game_id;

    // Persisted traits
    char local_address[40];
    char remote_address;
    char ipv6_address;
    char manual_address;
    unsigned char mac_address[8];
    char name[61];
    int has_custom_name;
    char uuid[37];
    void *server_cert;
    GList *app_list;
};

typedef struct NVCOMPUTER_T NVCOMPUTER;

void nvcomputer_free(NVCOMPUTER *p);