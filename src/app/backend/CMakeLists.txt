target_sources(moonlight-lib PRIVATE
        backend_root.c
        backend_gs.c
        pcmanager/pcmanager.c
        pcmanager/pairing.c
        pcmanager/pcmanager_common.c
        pcmanager/known_hosts.c
        pcmanager/pclist.c
        pcmanager/listeners.c
        pcmanager/worker/request.c
        pcmanager/worker/pairing.c
        pcmanager/worker/quit_app.c
        pcmanager/worker/wol.c
        pcmanager/worker/manual_add.c
        pcmanager/worker/update.c
        apploader/apploader.c)

add_subdirectory(pcmanager)