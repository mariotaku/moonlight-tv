#include "session_events.h"
#include "priv.h"


bool session_handle_input_event(session_t *session, const SDL_Event *event) {
    if (!session_accepting_input(session)) {
        return false;
    }
    stream_input_t *input = &session->input;
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            sdlinput_handle_key_event(input, &event->key);
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            sdlinput_handle_caxis_event(input, &event->caxis);
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            stream_input_handle_cbutton(input, &event->cbutton);
            break;
        }
        case SDL_MOUSEMOTION: {
            session_handle_mmotion_event(input, &event->motion);
            break;
        }
        case SDL_MOUSEWHEEL: {
            if (!input->view_only && !input->no_sdl_mouse) {
                sdlinput_handle_mwheel_event(&event->wheel);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            if (!input->view_only && !input->no_sdl_mouse) {
                sdlinput_handle_mbutton_event(&event->button);
            }
            break;
        }
        case SDL_TEXTINPUT: {
            sdlinput_handle_text_event(input, &event->text);
            break;
        }
        default:
            return false;
    }
    return true;
}
