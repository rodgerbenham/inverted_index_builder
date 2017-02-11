#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCKS 0
#define MAX_DOC_TITLE_LENGTH 500

struct TermDocs {
    GString *term;
    GSList *doc_ids;
} typedef term_docs_t;

void read_doc_file(int, char*, GSList*);
term_docs_t* generate_term_doc(char*, int); 
gint term_sort_comparator(gconstpointer, gconstpointer);
gint doc_sort_comparator(gconstpointer, gconstpointer);
gint key_term_comparator(char*, term_docs_t*);
void for_each_list_item(GSList*, void (*action)(GSList *list)); 
void collect_term_docs(GSList*);
void display_term_docs(GSList*);
void clear_term_docs(GSList*);
term_docs_t* bsearch_postings(char *, gpointer *, size_t,
        int(*compare)(char* key, term_docs_t* doc)); 

int
main (int argc, char* argv[]) {
    int document_id_counter = 1;

    for (int i = 0; i <= BLOCKS; i++) {
        struct dirent *dp;
        DIR *dfd;
        g_print("Starting block %d:\n", i);
        g_print("\tAllocation Phase\n");
        GSList *term_doc_list;
        GHashTable *doc_id_doc_name_map = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, g_free);
        term_doc_list = NULL;
        term_doc_list = g_slist_alloc();

        char dir[MAX_DOC_TITLE_LENGTH];
        sprintf(dir, "./docs/%d", i);
        
        if ((dfd = opendir(dir)) == NULL)
        {
            fprintf(stderr, "Can't open %s\n", dir);
            return -1;
        }

        char filename_qfd[MAX_DOC_TITLE_LENGTH] ;

        g_print("\tDocument load phase\n");
        while ((dp = readdir(dfd)) != NULL)
        {
            struct stat stbuf ;
            sprintf(filename_qfd, "%s/%s", dir, dp->d_name) ;
            if(stat(filename_qfd, &stbuf) == -1)
            {
                printf("Unable to stat file: %s\n",filename_qfd);
                continue;
            }

            if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
            {
                // Skip directories
                continue;
            }
            else
            {
                int *doc_id = malloc(sizeof(int));
                *doc_id = document_id_counter;

                g_hash_table_insert(doc_id_doc_name_map, (gpointer) doc_id, g_strdup(filename_qfd));
                read_doc_file(document_id_counter, filename_qfd, term_doc_list);
                document_id_counter++;
            }
        }
        closedir(dfd);

        g_print("\t%d terms loaded\n", g_slist_length(term_doc_list));
        g_print("\tSorting Phase\n");
        term_doc_list = g_slist_sort(term_doc_list, (GCompareFunc)term_sort_comparator);
        g_print("\tCollect Phase\n");
        for_each_list_item(term_doc_list, collect_term_docs);
        g_print("\tCleanup Phase\n");
        for_each_list_item(term_doc_list, clear_term_docs);
        g_slist_free(term_doc_list);
        g_hash_table_remove_all(doc_id_doc_name_map);
        g_hash_table_destroy(doc_id_doc_name_map);
    } 

    return 0;
}
term_docs_t *
bsearch_postings (char *key, gpointer *array, size_t num,
        int(*compare)(char* key, term_docs_t* doc)) {
    size_t start = 0, end = num;
    int result;

    while (start < end) {
        size_t mid = start + (end - start) / 2;
        term_docs_t* doc = (term_docs_t*) *(array + mid);

        result = compare(key, doc);
        if (result < 0)
            end = mid;
        else if (result > 0)
            start = mid + 1;
        else
            return *(array + mid);
    }

    return NULL;
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
            list = g_slist_insert(list, generate_term_doc(term->str, doc_id), 1);
            g_string_free(term, TRUE);
            term = g_string_new("");
            continue;
        }
        g_string_append_c(term, (char)c);
    }

    list = g_slist_insert(list, generate_term_doc(term->str, doc_id), 1);
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
key_term_comparator(char* key, term_docs_t* term_doc) {
    return g_ascii_strcasecmp(key, term_doc->term->str);
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
            // However, calling free here causes a core dump when Valgrind isn't running.
            // Leak is rather minimal (32 bytes) but will continue to monitor.
            // free(next);
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
