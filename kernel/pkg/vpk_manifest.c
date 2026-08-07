












#include "pkg/vpk_manifest.h"

static void copy_trimmed(char *dest, usize dest_size, const char *start, const char *end)
{
    


    while (start < end && (*start == ' ' || *start == '\t')) {
        start++;
    }
    while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\r')) {
        end--;
    }

    usize len = (usize)(end - start);
    if (len >= dest_size) {
        len = dest_size - 1;
    }
    for (usize i = 0; i < len; i++) {
        dest[i] = start[i];
    }
    dest[len] = '\0';
}

static bool starts_with(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s != *prefix) {
            return false;
        }
        s++;
        prefix++;
    }
    return true;
}

static const char *find_substr(const char *haystack, const char *haystack_end, const char *needle)
{
    usize needle_len = 0;
    while (needle[needle_len]) {
        needle_len++;
    }

    for (const char *p = haystack; p + needle_len <= haystack_end; p++) {
        bool match = true;
        for (usize i = 0; i < needle_len; i++) {
            if (p[i] != needle[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            return p;
        }
    }
    return NULL;
}

bool vpk_manifest_parse(const char *text, usize len, struct vpk_manifest *out)
{
    extern void printk(const char *fmt, ...);
    printk("vpk_manifest_parse: ENTER len=%llu\n", (u64)len);

    out->name[0] = '\0';
    out->version[0] = '\0';
    out->description[0] = '\0';
    out->depends_count = 0;
    out->files_count = 0;

    bool in_files_section = false;
    const char *line_start = text;
    const char *text_end = text + len;

    u32 line_iterations = 0;

    for (const char *p = text; p <= text_end; p++) {
        if (p == text_end || *p == '\n') {
            line_iterations++;
            if (line_iterations > 100) {
                extern __attribute__((noreturn)) void panic(const char *msg);
                printk("vpk_manifest_parse: ABORT, >100 lines processed -- infinite loop\n");
                panic("vpk_manifest_parse: line loop exceeded 100 iterations");
            }

            const char *line_end = p;
            usize line_len = (usize)(line_end - line_start);
            printk("vpk_manifest_parse: line #%u, len=%llu, in_files_section=%d\n",
                   line_iterations, (u64)line_len, (int)in_files_section);

            

            const char *content_start = line_start;
            while (content_start < line_end && (*content_start == ' ' || *content_start == '\t')) {
                content_start++;
            }
            if (content_start == line_end || *content_start == '\r') {
                line_start = p + 1;
                continue;
            }

            if (starts_with(content_start, "[files]")) {
                in_files_section = true;
                line_start = p + 1;
                continue;
            }

            if (in_files_section) {
                printk("vpk_manifest_parse: in [files] section, searching for '->'\n");
                const char *arrow = find_substr(content_start, line_end, "->");
                printk("vpk_manifest_parse: find_substr returned %s\n", arrow ? "found" : "NULL");
                if (arrow && out->files_count < VPK_MAX_FILE_MAPPINGS) {
                    struct vpk_file_mapping *fm = &out->files[out->files_count];
                    copy_trimmed(fm->source, sizeof(fm->source), content_start, arrow);
                    copy_trimmed(fm->dest, sizeof(fm->dest), arrow + 2, line_end);
                    printk("vpk_manifest_parse: file mapping added: \"%s\" -> \"%s\"\n",
                           fm->source, fm->dest);
                    out->files_count++;
                }
            } else {
                const char *colon = NULL;
                for (const char *q = content_start; q < line_end; q++) {
                    if (*q == ':') {
                        colon = q;
                        break;
                    }
                }

                if (colon) {
                    char key[32];
                    copy_trimmed(key, sizeof(key), content_start, colon);

                    if (starts_with(key, "name")) {
                        copy_trimmed(out->name, sizeof(out->name), colon + 1, line_end);
                    } else if (starts_with(key, "version")) {
                        copy_trimmed(out->version, sizeof(out->version), colon + 1, line_end);
                    } else if (starts_with(key, "description")) {
                        copy_trimmed(out->description, sizeof(out->description), colon + 1, line_end);
                    } else if (starts_with(key, "depends")) {
                        





                        const char *dep_start = colon + 1;
                        while (dep_start < line_end && (*dep_start == ' ' || *dep_start == '\t')) {
                            dep_start++;
                        }
                        while (dep_start < line_end && out->depends_count < VPK_MAX_DEPS) {
                            const char *comma = dep_start;
                            while (comma < line_end && *comma != ',') {
                                comma++;
                            }
                            copy_trimmed(out->depends[out->depends_count],
                                         sizeof(out->depends[out->depends_count]),
                                         dep_start, comma);
                            if (out->depends[out->depends_count][0] != '\0') {
                                out->depends_count++;
                            }
                            dep_start = (comma < line_end) ? comma + 1 : line_end;
                        }
                    }
                    





                }
            }

            line_start = p + 1;
        }
    }

    return out->name[0] != '\0' && out->version[0] != '\0';
}
