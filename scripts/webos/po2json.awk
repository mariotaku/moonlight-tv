#!/usr/bin/awk -f

BEGIN {
    ENTRY_COUNT = 0
    print ("{")
}

match($0, /^msgid "(.+)"$/, group) {
    ENTRY_KEY = group[1]
}

match($0, /^msgstr "(.+)"$/, group) {
    if (ENTRY_COUNT > 0) {
        printf (",\n")
    }
    printf ("  \"%s\": \"%s\"", ENTRY_KEY, group[1])
    ENTRY_COUNT++
}

END {
    print ("\n}")
}