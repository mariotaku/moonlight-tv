#include "session_events.h"
#include "session_priv.h"


bool session_handle_input_event(session_t *session, const SDL_Event *event) {
    if (!session_accepting_input(session)) {
        return false;
    }
    stream_input_t *input = &session->input;
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            stream_input_handle_key(input, &event->key);
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            stream_input_handle_caxis(input, &event->caxis);
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            stream_input_handle_cbutton(input, &event->cbutton);
            break;
        }
        case SDL_CONTROLLERSENSORUPDATE: {
            stream_input_handle_csensor(input, &event->csensor);
            break;
        }
        case SDL_CONTROLLERTOUCHPADDOWN:
        case SDL_CONTROLLERTOUCHPADMOTION:
        case SDL_CONTROLLERTOUCHPADUP: {
            stream_input_handle_ctouchpad(input, &event->ctouchpad);
            break;
        }
        case SDL_CONTROLLERDEVICEADDED: {
            stream_input_handle_cdevice(input, &event->cdevice);
            break;
        }
        case SDL_MOUSEMOTION: {
            stream_input_handle_mmotion(input, &event->motion);
            break;
        }
        case SDL_MOUSEWHEEL: {
            if (!input->view_only && !input->no_sdl_mouse) {
                stream_input_handle_mwheel(input, &event->wheel);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            if (!input->view_only && !input->no_sdl_mouse) {
                stream_input_handle_mbutton(input, &event->button);
            }
            break;
        }
        case SDL_TEXTINPUT: {
            stream_input_handle_text(input, &event->text);
            break;
        }
        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
        case SDL_FINGERMOTION: {
            stream_input_handle_touch(input, &event->tfinger);
            break;
        }
        default:
            return false;
    }
    return true;
}
