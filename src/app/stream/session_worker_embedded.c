/*
 * Copyright (c) 2024 Mariotaku <https://github.com/mariotaku>.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "session_worker.h"
#include "app.h"
#include "client.h"
#include "session_priv.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "errors.h"
#include "app_session.h"
#include "logging.h"
#include "backend/pcmanager/worker/worker.h"
#include "embed_wrapper.h"

#include <errno.h>


int session_worker_embedded(session_t *session) {
    app_t *app = session->app;
    session_set_state(session, STREAMING_CONNECTING);
    bus_pushevent(USER_STREAM_CONNECTING, NULL, NULL);
    streaming_error(session, GS_OK, "");


    embed_process_t *proc = embed_spawn(session->app_name, app->settings.key_dir, session->server->serverInfo.address,
                                        session->config.stream.width, session->config.stream.height,
                                        session->config.stream.fps, session->config.stream.bitrate,
                                        session->config.stream.audioConfiguration, session->config.local_audio,
                                        !session->config.sops, session->input.view_only);
    if (proc == NULL) {
        session_set_state(session, STREAMING_ERROR);
        streaming_error(session, GS_WRONG_STATE, "Failed to start embedded session: %s", strerror(errno));
        bus_pushevent(USER_STREAM_FINISHED, NULL, NULL);
        return -1;
    }
    session_set_state(session, STREAMING_STREAMING);
    bus_pushevent(USER_STREAM_OPEN, NULL, NULL);

    app_bus_post_sync(app, (bus_actionfunc) app_ui_close, &app->ui);

    SDL_LockMutex(session->mutex);
    session->embed_process = proc;
    SDL_UnlockMutex(session->mutex);

    char tmp[512], last_line[512] = "\0";
    while (!session->interrupted && embed_read(proc, tmp, sizeof(tmp))) {
        if (tmp[strlen(tmp) - 1] == '\n') {
            tmp[strlen(tmp) - 1] = '\0';
        }
        strncpy(last_line, tmp, sizeof(last_line));
        last_line[sizeof(last_line) - 1] = '\0';
        commons_log_info("Session", "moonlight-embedded: %s", tmp);
    }

    SDL_LockMutex(session->mutex);
    session->embed_process = NULL;
    SDL_UnlockMutex(session->mutex);

    int ret = embed_wait(proc);
    if (ret != 0) {
        if (WIFSIGNALED(ret)) {
            streaming_error(session, GS_WRONG_STATE, "moonlight-embedded exited with signal %d.\n"
                                                     "Last output: %s", WTERMSIG(ret), last_line);
        } else if (WIFEXITED(ret)) {
            streaming_error(session, GS_WRONG_STATE, "moonlight-embedded exited with code %d.\n"
                                                     "Last output: %s", WEXITSTATUS(ret), last_line);
        } else {
            streaming_error(session, GS_WRONG_STATE, "moonlight-embedded exited with unknown status %d.\n"
                                                     "Last output: %s", ret, last_line);
        }

    }

    bus_pushevent(USER_STREAM_CLOSE, NULL, NULL);

    session_set_state(session, STREAMING_DISCONNECTING);

    if (ret == 0) {
        PSERVER_DATA server = session->server;
        if (session->quitapp) {
            commons_log_info("Session", "Sending app quit request ...");
            GS_CLIENT client = app_gs_client_new(app);
            gs_set_timeout(client, 30);
            gs_quit_app(client, server);
            gs_destroy(client);
        }
        worker_context_t update_ctx = {
                .app = app,
                .manager = pcmanager,
        };
        uuidstr_fromstr(&update_ctx.uuid, server->uuid);
        pcmanager_update_by_host(&update_ctx, server->serverInfo.address, server->extPort, true);

        // Don't always reset status as error state should be kept
        session_set_state(session, STREAMING_NONE);
    }

    bus_pushevent(USER_STREAM_FINISHED, NULL, NULL);
    app_bus_post(app, (bus_actionfunc) app_session_destroy, app);
    return 0;
}

