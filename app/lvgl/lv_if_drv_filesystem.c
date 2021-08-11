/* MIT License
 * 
 * Copyright (c) [2020] [Ryan Wendland]
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "lv_conf.h"
#include "lv_if_drv_filesystem.h"

#if (LV_USE_FILESYSTEM == 1)

#if (LV_USE_USER_DATA == 0)
#error "LV_USE_USER_DATA must be set to 1 to use filesystem driver"
#endif

#if defined _WIN32 || defined __CYGWIN__
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#if LV_USE_FILESYSTEM_DIR_LISTING
#if defined _WIN32 || defined __CYGWIN__
#include <windows.h>
typedef HANDLE dir_t;
#else
#include <dirent.h>
typedef DIR *dir_t;
#endif
#endif

typedef FILE *file_t;

#define DEBUG_FS 0
#define debug_printf(fmt, ...)          \
    do                                  \
    {                                   \
        if (DEBUG_FS)                   \
            printf(fmt, ##__VA_ARGS__); \
    } while (0)

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_fs_res_t fs_open(lv_fs_drv_t *drv, void *file_p, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos);
static lv_fs_res_t fs_size(lv_fs_drv_t *drv, void *file_p, uint32_t *size_p);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);
static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_trunc(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname, const char *newname);
static lv_fs_res_t fs_free(lv_fs_drv_t *drv, uint32_t *total_p, uint32_t *free_p);
static lv_fs_res_t fs_dir_open(lv_fs_drv_t *drv, void *dir_p, const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p);

/**
 * Register a driver for the File system interface
 */
void lv_if_init_filesystem(const char *fs_id)
{
    char *cwd = malloc(256);
    lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    fs_drv.file_size = sizeof(file_t);
    fs_drv.letter = fs_id[0];
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;
    fs_drv.free_space_cb = fs_free;
    fs_drv.size_cb = fs_size;
    fs_drv.remove_cb = fs_remove;
    fs_drv.rename_cb = fs_rename;
    fs_drv.trunc_cb = fs_trunc;

#if LV_USE_FILESYSTEM_DIR_LISTING
    fs_drv.rddir_size = sizeof(dir_t);
    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;
    fs_drv.user_data = cwd;
    strcpy(cwd, "");
#endif

    lv_fs_drv_register(&fs_drv);
    debug_printf("Filesystem registered with identifier: %c\n", fs_drv.letter);
}

void lv_if_deinit_filesystem(const char *fs_id)
{
    char letter = fs_id[0];

    lv_fs_drv_t *drv = lv_fs_get_drv(letter);

    if (drv == NULL)
        return;

    if (drv->user_data == NULL)
        return;

    free(drv->user_data);
}

static void change_path_separator(char *path)
{
    const char to = PATH_SEPARATOR;
    const char from = (to == '\\') ? '/' : '\\';

    char *current_pos = strchr(path, from);
    while (current_pos)
    {
        *current_pos = to;
        current_pos = strchr(current_pos, from);
    }
}

/**
 * Open a file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable
 * @param path path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_open(lv_fs_drv_t *drv, void *file_p, const char *path, lv_fs_mode_t mode)
{
    (void)drv;

    const char *flags = "";
    if (mode == LV_FS_MODE_WR)
        flags = "wb";
    else if (mode == LV_FS_MODE_RD)
        flags = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
        flags = "rb+";

    char *cwd = drv->user_data;

    //Build and condition the full path
    char full_path[256];
    sprintf(full_path, "%s%s", cwd, path);
    change_path_separator(full_path);

    debug_printf("Opening %s with flags %s...", full_path, flags);
    file_t f = fopen(full_path, flags);
    if (f == NULL)
    {
        debug_printf("Error!\n");
        return LV_FS_RES_FS_ERR;
    }
    debug_printf("Ok!\n");
    fseek(f, 0, SEEK_SET);

    file_t *fp = file_p;
    *fp = f;

    return LV_FS_RES_OK;
}

/**
 * Close an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable. (opened with lv_ufs_open)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    debug_printf("Closing opened file\n");
    file_t *fp = file_p;
    fclose(*fp);
    return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable.
 * @param buf pointer to a memory block where to store the read data
 * @param btr number of Bytes To Read
 * @param br the real number of read bytes (Byte Read)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    (void)drv;
    file_t *fp = file_p;

    if (fp != NULL)
    {
        debug_printf("Reading %i bytes from file...", btr);
        int bytes_read = fread(buf, 1, btr, *fp);
        debug_printf("Read %i bytes!\n", bytes_read);
        if (br != NULL)
            *br = bytes_read;
    }
    else
    {
        debug_printf("Error in fs_read: File not opened\n");
        return LV_FS_RES_FS_ERR;
    }

    return LV_FS_RES_OK;
}

/**
 * Write into a file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable
 * @param buf pointer to a buffer with the bytes to write
 * @param btr Bytes To Write
 * @param br the number of real written bytes (Bytes Written). NULL if unused.
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    (void)drv;
    file_t *fp = file_p;
    debug_printf("Writing %i bytes to file...", btw);
    int bytes_written = fwrite(buf, 1, btw, *fp);
    debug_printf("Wrote %i bytes!\n", bytes_written);
    if (bw != NULL)
        *bw = bytes_written;
    return LV_FS_RES_OK;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable. (opened with lv_ufs_open )
 * @param pos the new position of read write pointer
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos)
{
    (void)drv;
    file_t *fp = file_p;
    fseek(*fp, pos, SEEK_SET);
    return LV_FS_RES_OK;
}

/**
 * Give the size of a file bytes
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable
 * @param size pointer to a variable to store the size
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_size(lv_fs_drv_t *drv, void *file_p, uint32_t *size_p)
{
    (void)drv;
    file_t *fp = file_p;

    uint32_t cur = ftell(*fp);

    fseek(*fp, 0L, SEEK_END);
    *size_p = ftell(*fp);

    /*Restore file pointer*/
    fseek(*fp, cur, SEEK_SET);

    return LV_FS_RES_OK;
}
/**
 * Give the position of the read write pointer
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable.
 * @param pos_p pointer to to store the result
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    file_t *fp = file_p;
    *pos_p = ftell(*fp);
    return LV_FS_RES_OK;
}

/**
 * Delete a file
 * @param drv pointer to a driver where this function belongs
 * @param path path of the file to delete
 * @return  LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path)
{
    (void)drv;
    (void)path;
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;
    debug_printf("fs_remove not implemented\n");
    assert(0);
    return res;
}

/**
 * Truncate the file size to the current position of the read write pointer
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to an 'ufs_file_t' variable. (opened with lv_fs_open )
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_trunc(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    (void)file_p;
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;
    debug_printf("fs_trunc not implemented\n");
    assert(0);
    return res;
}

/**
 * Rename a file
 * @param drv pointer to a driver where this function belongs
 * @param oldname path to the file
 * @param newname path with the new name
 * @return LV_FS_RES_OK or any error from 'fs_res_t'
 */
static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname, const char *newname)
{
    (void)drv;
    static char new[256];
    static char old[256];

    char *cwd = drv->user_data;

    //Build and condition the full path
    sprintf(old, "%s%s", cwd, oldname);
    change_path_separator(old);

    sprintf(new, "%s%s", cwd, newname);
    change_path_separator(new);

    int r = rename(old, new);

    if (r == 0)
        return LV_FS_RES_OK;
    else
        return LV_FS_RES_UNKNOWN;
}

/**
 * Get the free and total size of a driver in kB
 * @param drv pointer to a driver where this function belongs
 * @param letter the driver letter
 * @param total_p pointer to store the total size [kB]
 * @param free_p pointer to store the free size [kB]
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_free(lv_fs_drv_t *drv, uint32_t *total_p, uint32_t *free_p)
{
    (void)drv;
    (void)total_p;
    (void)free_p;
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;
    debug_printf("fs_free not implemented\n");
    assert(0);
    return res;
}

/**
 * Initialize a 'fs_read_dir_t' variable for directory reading
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to a 'fs_read_dir_t' variable
 * @param path path to a directory
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_open(lv_fs_drv_t *drv, void *dir_p, const char *path)
{
    (void)drv;

    char *cwd = drv->user_data;

    //Append the folder to the root working_dir
    sprintf(cwd, "%s%s", cwd, path);
    change_path_separator(cwd);

    //Add a path separator on the end of the string if the user didnt have one
    int len = strlen(cwd);
    if (len < (256 - 1) && cwd[len - 1] != PATH_SEPARATOR)
    {
        cwd[len] = PATH_SEPARATOR;
        cwd[len + 1] = '\0';
    }

#if LV_USE_FILESYSTEM_DIR_LISTING
    dir_t d;
    #if defined _WIN32 || defined __CYGWIN__
    d = INVALID_HANDLE_VALUE;
    #else
    if ((d = opendir(cwd)) == NULL)
    {
        debug_printf("%s directory opened\n", cwd);
        return LV_FS_RES_FS_ERR;
    }
    #endif

    dir_t *dp = dir_p;
    *dp = d;
    debug_printf("%s directory opened\n", cwd);
    return LV_FS_RES_OK;
#else
    (void)dir_p;
    return LV_FS_RES_NOT_IMP;
#endif
}

/*
 * Read the next filename form a directory.
 * The name of the directories will begin with '/'
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to an initialized 'fs_read_dir_t' variable
 * @param fn pointer to a buffer to store the filename
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn)
{
    (void)drv;
#if LV_USE_FILESYSTEM_DIR_LISTING
    dir_t *dp = dir_p;

    char *cwd = drv->user_data;

    #if defined _WIN32 || defined __CYGWIN__
    static WIN32_FIND_DATA fdata;
    int result = 1;
    strcpy(fn, "");

    //First time searching for a file we need to call FindFirstFile
    if (*dp == INVALID_HANDLE_VALUE)
    {
        char search_path[256];
        sprintf(search_path, "%s%c", cwd, '*');
        debug_printf("Search path %s\n", search_path);
        *dp = FindFirstFile(search_path, &fdata);
        if (*dp == INVALID_HANDLE_VALUE)
        {
            debug_printf("%s directory not found\n", cwd);
            return LV_FS_RES_FS_ERR;
        }
    }
    else if (FindNextFile(*dp, &fdata) == 0)
    {
        debug_printf("No more files in %s\n", cwd);
        return LV_FS_RES_FS_ERR;
    }

    do
    {
        if (strcmp(fdata.cFileName, ".") == 0 || strcmp(fdata.cFileName, "..") == 0)
        {
            continue;
        }
        else
        {
            if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                sprintf(fn, "/%s", fdata.cFileName);
            else
                sprintf(fn, "%s", fdata.cFileName);
            break;
        }
    } while (FindNextFile(*dp, &fdata) != 0);

    #else
    struct dirent *entry;
    do
    {
        entry = readdir(*dp);
        if (entry)
        {
            if (entry->d_type == DT_DIR)
                sprintf(fn, "/%s", entry->d_name);
            else
                strcpy(fn, entry->d_name);
        }
        else
        {
            strcpy(fn, "");
            debug_printf("No more files in %s\n", cwd);
            return LV_FS_RES_FS_ERR;
        }
    } while (strcmp(fn, "/.") == 0 || strcmp(fn, "/..") == 0);
    #endif
    return LV_FS_RES_OK;
#else
    (void)dir_p;
    (void)fn;
    return LV_FS_RES_NOT_IMP; 
#endif
}

/**
 * Close the directory reading
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to an initialized 'fs_read_dir_t' variable
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p)
{
    (void)drv;

#if LV_USE_FILESYSTEM_DIR_LISTING
    dir_t *dp = dir_p;

    char *cwd = drv->user_data;
    debug_printf("Closing dir %s\n", cwd);
    lv_fs_up(cwd);
    debug_printf("Final working dir %s\n", cwd);

    if (drv->user_data == NULL)
        return LV_FS_RES_UNKNOWN;

    #if defined _WIN32 || defined __CYGWIN__
    FindClose(*dp);
    *dp = INVALID_HANDLE_VALUE;
    #else
    closedir(*dp);
    #endif
    return LV_FS_RES_OK;
#else
    (void)dir_p;
    return LV_FS_RES_NOT_IMP;
#endif
}

#endif /*LV_USE_FILESYSTEM*/
