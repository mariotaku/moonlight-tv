#include "nuklear/platform_sprites.h"
#include "nuklear/ext_sprites.h"

#include "ui/root.h"

struct nk_image ic_navkey_cancel()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_back;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_b;
    }
};
struct nk_image ic_navkey_close()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_red;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_x;
    }
};

struct nk_image ic_navkey_menu()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_yellow;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_view;
    }
};

struct nk_image ic_navkey_confirm()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_center;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_a;
    }
};