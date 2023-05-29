install(FILES deploy/linux/moonlight-tv.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES deploy/linux/moonlight-tv.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps)
install(DIRECTORY ${CMAKE_BINARY_DIR}/mo/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_NAME "moonlight-tv")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
set(CPACK_DEBIAN_PACKAGE_SUMMARY "Open Source NVIDIA GameStream Client")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mariotaku Lee <mariotaku.lee@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/mariotaku/moonlight-tv")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
# set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
set(CPACK_DEBIAN_PACKAGE_DEPENDS_LIST
        "libc6 (>= 2.28)"
        "libsdl2-2.0-0 (>= 2.0.4)"
        "libsdl2-image-2.0-0 (>= 2.0.1)"
        "libopus0 (>= 1.1)"
        "libcurl4 (>= 7.16.2)"
        "libuuid1 (>= 2.16)"
        "libexpat1 (>= 2.0.1)"
        "libmbedcrypto3 (>= 2.13)"
        "libmbedx509-0 (>= 2.0)"
        )

if (TARGET ss4s-module-mmal)
    list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS_LIST "libraspberrypi0")
endif ()

if (TARGET ss4s-module-ffmpeg)
    list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS_LIST
            "libavcodec58 (>= 7:4.0)"
            "libavutil56 (>= 7:4.0)"
            )
endif ()

set(CPACK_DEBIAN_PACKAGE_RECOMMENDS_LIST
        "libasound2 (>= 1.0.16)"
        "libpulse0 (>= 0.99.1)"
        "fonts-dejavu-core"
        )
set(CPACK_DEBIAN_PACKAGE_SUGGESTS_LIST
        "libcec6 (>= 6.0)"
        )
string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS_LIST}")
string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_RECOMMENDS "${CPACK_DEBIAN_PACKAGE_RECOMMENDS_LIST}")
string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_SUGGESTS "${CPACK_DEBIAN_PACKAGE_SUGGESTS_LIST}")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

include(CPack)