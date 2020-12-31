#pragma once

#include <glib.h>

enum MOONLIGHT_COMPUTER_STATE
{
    CS_UNKNOWN,
    CS_ONLINE,
    CS_OFFLINE
};
typedef enum MOONLIGHT_COMPUTER_STATE MOONLIGHT_COMPUTER_STATE;

enum MOONLIGHT_PAIR_STATE
{
    PS_UNKNOWN,
    PS_PAIRED,
    PS_NOT_PAIRED
};
typedef enum MOONLIGHT_PAIR_STATE MOONLIGHT_PAIR_STATE;

struct MOONLIGHT_COMPUTER_T
{
    // Ephemeral traits
    MOONLIGHT_COMPUTER_STATE state;
    MOONLIGHT_PAIR_STATE pair_state;
    char activeAddress[40];
    int currentGameId;

    // Persisted traits
    char localAddress[40];
    char name[60];
    GList *appList;
};

typedef struct MOONLIGHT_COMPUTER_T MOONLIGHT_COMPUTER;