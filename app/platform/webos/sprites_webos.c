#include "nuklear/platform_sprites.h"
#include "nuklear/ext_sprites.h"

#include "ui/root.h"

struct nk_image ic_navkey_cancel()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_b;
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
    case UI_INPUT_MODE_REMOTE:
    default:
        return sprites_ui.ic_remote_back;
    }
}
struct nk_image ic_navkey_close()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_x;
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
    case UI_INPUT_MODE_REMOTE:
    default:
        return sprites_ui.ic_remote_red;
    }
}

struct nk_image ic_navkey_menu()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_view;
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
    case UI_INPUT_MODE_REMOTE:
    default:
        return sprites_ui.ic_remote_yellow;
    }
}

struct nk_image ic_navkey_confirm()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_a;
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
    case UI_INPUT_MODE_REMOTE:
    default:
        return sprites_ui.ic_remote_center;
    }
}