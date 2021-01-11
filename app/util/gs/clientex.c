#include "libgamestream/client.c"

int gs_download_cover(PSERVER_DATA server, int appid, const char *path)
{
    int ret = GS_OK;
    char url[4096];
    uuid_t uuid;
    char uuid_str[37];
    PHTTP_DATA data = http_create_data();
    if (data == NULL)
        return GS_OUT_OF_MEMORY;

    uuid_generate_random(uuid);
    uuid_unparse(uuid, uuid_str);
    snprintf(url, sizeof(url), "https://%s:47984/appasset?uniqueid=%s&uuid=%s&appid=%d&AssetType=2&AssetIdx=0",
             server->serverInfo.address, unique_id, uuid_str, appid);
    ret = http_request(url, data);
    if (ret != GS_OK)
        goto cleanup;

    FILE *f = fopen(path, "wb");
    if (!f)
    {
        ret = GS_IO_ERROR;
        goto cleanup;
    }

    fwrite(data->memory, data->size, 1, f);
    fflush(f);
    fclose(f);

cleanup:
    http_free_data(data);
    return ret;
}