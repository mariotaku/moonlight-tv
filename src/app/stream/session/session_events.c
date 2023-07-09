#include "session_events.h"
#include "priv.h"
#include "stream/input/sdlinput.h"


bool session_handle_input_event(session_t *session, const SDL_Event *event) {
    if (!session_input_should_accept(&session->input)) {
        return false;
    }
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sdlinput_handle_key_event(&session->input,&event->key);
            break;
        case SDL_CONTROLLERAXISMOTION:
            sdlinput_handle_caxis_event(&session->input, &event->caxis);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            stream_input_handle_cbutton(&session->input, &event->cbutton);
            break;
        case SDL_MOUSEMOTION:
            session_handle_mmotion_event(&session->input, &event->motion);
            break;
        case SDL_MOUSEWHEEL:
            if (!absinput_no_control && !absinput_no_sdl_mouse) {
                sdlinput_handle_mwheel_event(&event->wheel);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (!absinput_no_control && !absinput_no_sdl_mouse) {
                sdlinput_handle_mbutton_event(&event->button);
            }
            break;
        case SDL_TEXTINPUT:
            sdlinput_handle_text_event(&event->text);
            break;
        default:
            return false;
    }
    return true;
}
