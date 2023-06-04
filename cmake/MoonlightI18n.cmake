find_program(MSGFMT_COMMAND msgfmt)
find_program(XGETTEXT_COMMAND xgettext)
find_program(MSGMERGE_COMMAND msgmerge)

if (NOT MSGFMT_COMMAND OR NOT XGETTEXT_COMMAND OR NOT MSGMERGE_COMMAND)
    message(FATAL_ERROR "msgfmt not found. Please install gettext.")
endif ()
set(MOFILES)
foreach (LANG ${I18N_LOCALES})
    string(REPLACE "-" "_" _MOLANG ${LANG})
    set(_POFILE ${CMAKE_SOURCE_DIR}/i18n/${LANG}/messages.po)
    set(_MODIR ${CMAKE_BINARY_DIR}/mo/${_MOLANG}/LC_MESSAGES)
    set(_MOFILE ${_MODIR}/moonlight-tv.mo)
    add_custom_command(OUTPUT ${_MOFILE} DEPENDS i18n/${LANG}/messages.po
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_MODIR}
            COMMAND ${MSGFMT_COMMAND} --output-file=${_MOFILE} ${_POFILE})
    list(APPEND MOFILES ${_MOFILE})
endforeach ()
add_custom_target(moonlight-i18n ALL DEPENDS ${MOFILES})
add_dependencies(moonlight moonlight-i18n)

add_custom_target(i18n-update-pot
        COMMAND ${XGETTEXT_COMMAND} --keyword=locstr --keyword=translatable --add-comments -o i18n/messages.pot
        ${SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(i18n-merge-po
        COMMAND ${MSGMERGE_COMMAND} --update i18n/ja/messages.po i18n/messages.pot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
