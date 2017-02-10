#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#define DOCUMENT_COUNT 4
#define MAX_DOC_PATH 20

struct TermDoc {
    GString *term;
    int doc;
} typedef term_doc_t;

void get_doc_path(int, char*);
void read_doc_file(int, char*, GSList*);
term_doc_t* generate_term_doc(char*, int); 
void display_term_docs(GSList*);
void clear_term_docs(GSList*);

int
main (int argc, char* argv[]) {
    GSList *termDocList;
    termDocList = NULL;
    termDocList = g_slist_alloc();
    for (int i = 1; i <= DOCUMENT_COUNT; i++) {
        char path[MAX_DOC_PATH];

        get_doc_path(i, path);
        read_doc_file(i, path, termDocList);
    }

    display_term_docs(termDocList);
    clear_term_docs(termDocList);
    g_slist_free(termDocList);
    return 0;
}

void
get_doc_path(int doc_id, char *path) {
    sprintf(path, "./docs/doc%d", doc_id);
}

void
read_doc_file(int doc_id, char* path, GSList *list) {
    int c;
    FILE *file;
    file = fopen(path, "r");
    GString *term;
    term = g_string_new("");
    while ((c = getc(file)) != EOF) {
        if (c == ' ') {
            list = g_slist_append(list, generate_term_doc(term->str, doc_id));
            g_string_free(term, TRUE);
            term = g_string_new("");
            continue;
        }
        g_string_append_c(term, (char)c);
    }

    list = g_slist_append (list, generate_term_doc(term->str, doc_id));
    g_string_free(term, TRUE);

    fclose(file);
}

term_doc_t*
generate_term_doc(char* term, int doc_id) {
    term_doc_t *t_doc = malloc(sizeof(term_doc_t));
    t_doc->term = g_string_new(g_strstrip(term));
    t_doc->doc = doc_id;
    return t_doc;
}

void
display_term_docs(GSList *list) {
    int nIndex;
    GSList *node = list;

    while ((node = node->next) != NULL) {
        g_print("data = %s\n", ((term_doc_t*) (node->data))->term->str);
    }
}

void
clear_term_docs(GSList *list) {
    int nIndex;
    GSList *node = list;

    while ((node = node->next) != NULL) {
        g_string_free(((term_doc_t *) (node->data))->term, TRUE);
        free(((term_doc_t *) (node->data)));
    }
}
