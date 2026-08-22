#!/usr/bin/awk -f
#
# Converts a gettext .po catalog into the flat JSON that webOS's resBundle
# loads from resources/<lang>[/<region>]/cstrings.json.
#
# Entries with an empty msgstr are skipped, so an untranslated string falls
# back to its msgid (English) at runtime instead of rendering blank.
#
# Continuation lines have to be joined. gettext splits long strings across
# several quoted lines, but the C compiler concatenates the literals, so
# locstr() looks up the joined string at runtime. Emitting only the first
# line produces a key that can never match, silently dropping the
# translation for exactly the longest messages.

function unquote(line,   first) {
    first = index(line, "\"")
    return substr(line, first + 1, length(line) - first - 1)
}

function flush(   sep) {
    # The header entry has an empty msgid and carries the .po metadata in its
    # msgstr; skipping empty keys drops it along with untranslated entries.
    if (key != "" && val != "") {
        sep = (count > 0) ? ",\n" : ""
        printf("%s  \"%s\": \"%s\"", sep, key, val)
        count++
    }
    key = ""
    val = ""
    state = ""
}

BEGIN {
    count = 0
    key = ""
    val = ""
    state = ""
    print ("{")
}

# Tolerate CRLF checkouts, which would otherwise leave a stray carriage
# return inside the emitted JSON string.
{ sub(/\r$/, "") }

/^msgid[ \t]+"/ {
    flush()
    key = unquote($0)
    state = "id"
    next
}

/^msgstr[ \t]+"/ {
    val = unquote($0)
    state = "str"
    next
}

/^"/ {
    if (state == "id") {
        key = key unquote($0)
    } else if (state == "str") {
        val = val unquote($0)
    }
    next
}

# Anything else - blank line, comment, obsolete entry - ends the current one.
{ flush() }

END {
    flush()
    print ("\n}")
}
