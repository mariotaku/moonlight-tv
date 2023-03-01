target_sources(moonlight PRIVATE
        backend_root.c
        pcmanager/pcmanager.c
        pcmanager/pairing.c
        pcmanager/pcmanager_common.c
        pcmanager/known_hosts.c
        pcmanager/discovery.c
        pcmanager/pclist.c
        pcmanager/listeners.c
        pcmanager/worker/request.c
        pcmanager/worker/pairing.c
        pcmanager/worker/quit_app.c
        pcmanager/worker/wol.c
        pcmanager/worker/manual_add.c
        pcmanager/worker/update.c
        gamecontrollerdb_updater.c
        apploader/apploader.c)