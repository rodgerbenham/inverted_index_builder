#include <stdio.h>
#include <glib.h>

#define DOCUMENT_COUNT 4
#define MAX_DOC_PATH 20

void get_doc_path(int, char*);
void read_doc_file(char*, GSList*);
void display_terms(GSList*);

struct TermDoc {
    GString term;
    int doc;
} typedef term_doc;

int
main (int argc, char* argv[]) {
    GSList *termDocList;
    termDocList = NULL;
    termDocList = g_slist_alloc();
    for (int i = 1; i <= DOCUMENT_COUNT; i++) {
        char path[MAX_DOC_PATH];

        get_doc_path(i, path);
        read_doc_file(path, termDocList);
        g_print("List count = %d\n", g_slist_length(termDocList));
    }

    display_terms(termDocList);
    //g_slist_free_full(termDocList, g_free);
    return 0;
}

void
get_doc_path(int id, char *path) {
    sprintf(path, "./docs/doc%d", id);
}

void
read_doc_file(char* path, GSList *list) {
    int c;
    FILE *file;
    file = fopen(path, "r");
    GString *term;
    term = g_string_new("");
    while ((c = getc(file)) != EOF) {
        if (c == ' ') {
            list = g_slist_append(list, g_string_new(term->str));
            g_string_free(term, TRUE);
            term = g_string_new("");
            continue;
        }
        g_string_append_c(term, (char)c);
    }

    list = g_slist_append (list, g_string_new(term->str));
    g_string_free(term, TRUE);

    fclose(file);
}

void
display_terms(GSList *list) {
    int nIndex;
    gpointer node;
    printf("Display terms called\n");
    g_print("Last data = %s\n", ((GString *) (g_slist_last(list)->data))->str);
}
