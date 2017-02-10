#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#define DOCUMENT_COUNT 4
#define MAX_DOC_PATH 20

struct TermDocs {
    GString *term;
    GSList *doc_ids;
} typedef term_docs_t;

void get_doc_path(int, char*);
void read_doc_file(int, char*, GSList*);
term_docs_t* generate_term_doc(char*, int); 
gint term_sort_comparator(gconstpointer, gconstpointer);
gint doc_sort_comparator(gconstpointer, gconstpointer);
void for_each_list_item(GSList*, void (*action)(GSList *list)); 
void collect_term_docs(GSList*);
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

    g_print("=== Unsorted ===\n\n");
    for_each_list_item(termDocList, display_term_docs);
    g_print("=== Sorted ===\n\n");
    termDocList = g_slist_sort(termDocList, (GCompareFunc)term_sort_comparator);
    for_each_list_item(termDocList, display_term_docs);
    g_print("=== Collect ===\n\n");
    for_each_list_item(termDocList, collect_term_docs);
    for_each_list_item(termDocList, display_term_docs);

    for_each_list_item(termDocList, clear_term_docs);
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

    list = g_slist_append(list, generate_term_doc(term->str, doc_id));
    g_string_free(term, TRUE);

    fclose(file);
}

term_docs_t*
generate_term_doc(char* term, int doc_id) {
    term_docs_t *t_doc = malloc(sizeof(term_docs_t));
    t_doc->term = g_string_new(g_strstrip(term));
    t_doc->doc_ids = NULL;
    t_doc->doc_ids = g_slist_append(t_doc->doc_ids, GINT_TO_POINTER(doc_id));
    return t_doc;
}

gint
term_sort_comparator(gconstpointer item1, gconstpointer item2) {
    term_docs_t* term_doc_1 = (term_docs_t*) item1; 
    term_docs_t* term_doc_2 = (term_docs_t*) item2; 
    if (term_doc_1 != NULL && term_doc_2 != NULL) {
        // CASE: Normal case
        return g_ascii_strcasecmp(term_doc_1->term->str, term_doc_2->term->str);
    }

    // CASE: Initial allocation of the list where data = NULL.
    // Workaround for list falling out of scope in read_doc_file due to g_slist_alloc.
    return -1;
}

gint
doc_sort_comparator(gconstpointer item1, gconstpointer item2) {
    int doc1 = GPOINTER_TO_INT(item1); 
    int doc2 = GPOINTER_TO_INT(item2); 
    return doc1 - doc2;
}

void
for_each_list_item(GSList *list, void (*action)(GSList *list)) {
    int nIndex;
    GSList *node = list;

    while ((node = node->next) != NULL) {
        (*action)(node);
    }
}

void
display_term_docs(GSList *node) {
    term_docs_t* term_doc = (term_docs_t*) node->data; 
    g_print("term = %s, doc = ", term_doc->term->str);
    GSList *next = term_doc->doc_ids;
    g_print("%d ", GPOINTER_TO_INT(next->data));

    while ((next = next->next) != NULL) {
        g_print("%d ", GPOINTER_TO_INT(next->data));
    }
    g_print("\n");
}

void
collect_term_docs(GSList *node) {
    term_docs_t* term_doc = (term_docs_t*) node->data; 
    GSList* next = NULL; 
    while ((next = node->next) != NULL) {
        term_docs_t* next_term_doc = NULL; 
        next_term_doc = (term_docs_t*) next->data;
        if (term_sort_comparator(term_doc, next_term_doc) == 0) {
            if (g_slist_find (term_doc->doc_ids, next_term_doc->doc_ids->data) == NULL) {
                term_doc->doc_ids = g_slist_insert_sorted(term_doc->doc_ids,
                    next_term_doc->doc_ids->data, (GCompareFunc)doc_sort_comparator);
            }
            
            node->next = next->next;

            clear_term_docs(next);
            // Valgrind wants free(next) to be called
            // And allows the program to run without error.
            // However, calling free here causes a core dump.
        }
        else {
            break;
        }
    }

    node->next = next;
}

void
clear_term_docs(GSList *node) {
    term_docs_t* term_docs = (term_docs_t *)node->data;
    g_string_free(term_docs->term, TRUE);
    g_slist_free(term_docs->doc_ids);
    g_free(term_docs);
}
