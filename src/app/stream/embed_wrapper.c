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

#include "embed_wrapper.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <Limelight.h>

struct embed_process_t {
    FILE *output;
    pid_t pid;
};

int embed_check_version(version_info_t *version_info) {
    FILE *f = popen("moonlight help 2>/dev/null", "r");
    if (f == NULL) {
        return -1;
    }
    // Get first line of output
    char buf[64];
    if (fgets(buf, sizeof(buf), f) == NULL) {
        pclose(f);
        return -1;
    }
    buf[sizeof(buf) - 1] = '\0';
    int ret = pclose(f);
    if (ret != 0) {
        return ret;
    }
    if (strncmp(buf, "Moonlight Embedded ", 19) != 0) {
        return -1;
    }
    return version_info_parse(version_info, buf + 19);
}

embed_process_t *embed_spawn(const char *app_name, const char *key_dir, const char *server_address,
                             int width, int height, int fps, int bitrate, int surround,
                             bool localaudio, bool nosops, bool viewonly) {
    int fds[2];
    if (pipe(fds) != 0) {
        return NULL;
    }

    char args_tmp[4096];
    size_t args_tmp_start = 0;
    char *app_name_tmp = args_tmp;
    args_tmp_start += snprintf(app_name_tmp, sizeof(args_tmp), "%s", app_name) + 1;
    char *key_dir_tmp = args_tmp + args_tmp_start;
    args_tmp_start += snprintf(key_dir_tmp, sizeof(args_tmp) - args_tmp_start, "%s", key_dir) + 1;
    char *width_tmp = args_tmp + args_tmp_start;
    args_tmp_start += snprintf(width_tmp, sizeof(args_tmp) - args_tmp_start, "%d", width) + 1;
    char *height_tmp = args_tmp + args_tmp_start;
    args_tmp_start += snprintf(height_tmp, sizeof(args_tmp) - args_tmp_start, "%d", height) + 1;
    char *fps_tmp = args_tmp + args_tmp_start;
    args_tmp_start += snprintf(fps_tmp, sizeof(args_tmp) - args_tmp_start, "%d", fps) + 1;
    char *bitrate_tmp = args_tmp + args_tmp_start;
    args_tmp_start += snprintf(bitrate_tmp, sizeof(args_tmp) - args_tmp_start, "%d", bitrate) + 1;
    char *server_address_tmp = args_tmp + args_tmp_start;
    snprintf(server_address_tmp, sizeof(args_tmp) - args_tmp_start, "%s", server_address);


    pid_t pid = fork();
    if (pid == -1) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }

    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);

        char *argv[64];
        int argc = 0;
        argv[argc++] = "moonlight";
        argv[argc++] = "stream";
        argv[argc++] = "-app";
        argv[argc++] = app_name_tmp;
        argv[argc++] = "-keydir";
        argv[argc++] = key_dir_tmp;
        argv[argc++] = "-width";
        argv[argc++] = width_tmp;
        argv[argc++] = "-height";
        argv[argc++] = height_tmp;
        argv[argc++] = "-fps";
        argv[argc++] = fps_tmp;
        argv[argc++] = "-bitrate";
        argv[argc++] = bitrate_tmp;
        if (localaudio) {
            argv[argc++] = "-localaudio";
        }
        if (viewonly) {
            argv[argc++] = "-viewonly";
        }
        if (nosops) {
            argv[argc++] = "-nosops";
        }
        switch (surround) {
            case AUDIO_CONFIGURATION_51_SURROUND:
                argv[argc++] = "-surround";
                argv[argc++] = "5.1";
                break;
            case AUDIO_CONFIGURATION_71_SURROUND:
                argv[argc++] = "-surround";
                argv[argc++] = "7.1";
                break;
            default:
                break;
        }
        argv[argc++] = server_address_tmp;
        argv[argc++] = NULL;
        execvp("moonlight", argv);
        exit(0);
    }
    close(fds[1]);

    FILE *f = fdopen(fds[0], "r");
    if (f == NULL) {
        close(fds[0]);
        kill(pid, SIGTERM);
        return NULL;
    }
    embed_process_t *process = malloc(sizeof(embed_process_t));
    assert(process != NULL);
    process->output = f;
    process->pid = pid;
    return process;
}

char *embed_read(embed_process_t *proc, char *buf, size_t size) {
    return fgets(buf, (int) size, proc->output);
}

int embed_interrupt(embed_process_t *proc) {
    return kill(proc->pid, SIGINT);
}

int embed_wait(embed_process_t *proc) {
    int status;
    waitpid(proc->pid, &status, 0);
    return status;
}