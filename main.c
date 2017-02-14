#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCKS 9
#define MAX_DOC_TITLE_LENGTH 500

struct TermDocs {
    int term_id;
    GSList *doc_ids;
} typedef term_docs_t;

typedef int bool;

void write_mapping(gpointer, gpointer, gpointer);
void read_doc_file(int, int*, char*, GSList*);
int generate_term_mapping(char*, int*); 
term_docs_t* deserialize_term_doc(FILE*, bool);
term_docs_t* generate_term_doc(int, int); 
gint term_sort_comparator(gconstpointer, gconstpointer);
gint doc_sort_comparator(gconstpointer, gconstpointer);
gint key_term_comparator(char*, term_docs_t*);
void for_each_list_item(GSList*, void (*action)(GSList *list)); 
void collect_term_docs(GSList*);
void display_term_docs(GSList*);
void clear_term_docs(GSList*);

GHashTable *id_to_term_map = NULL; 
GHashTable *term_to_id_map = NULL; 

int
main (int argc, char* argv[]) {
    int document_id_counter = 0;
    int term_id_counter = 0;
    
    GHashTable *doc_map = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    id_to_term_map = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    term_to_id_map = g_hash_table_new(g_str_hash, g_str_equal);

    for (int i = 0; i <= BLOCKS; i++) {
        struct dirent *dp;
        DIR *dfd;
        g_print("Starting block %d:\n", i);
        g_print("\tAllocation Phase\n");
        
        GSList *term_doc_list;
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
                document_id_counter++;
                g_hash_table_insert(doc_map, GINT_TO_POINTER(document_id_counter), 
                        g_strdup(filename_qfd));
                read_doc_file(document_id_counter, &term_id_counter, filename_qfd, term_doc_list);
            }
        }
        closedir(dfd);

        g_print("\t%d terms loaded\n", g_slist_length(term_doc_list));
        g_print("\tSorting Phase\n");
        term_doc_list = g_slist_sort(term_doc_list, (GCompareFunc)term_sort_comparator);
        g_print("\tCollect Phase\n");
        for_each_list_item(term_doc_list, collect_term_docs);
        g_print("\tWrite Intermediate File Phase\n");
        
        char i_file_path[MAX_DOC_TITLE_LENGTH];
        sprintf(i_file_path, "./intermediates/%d.intermediate", i);

        FILE* i_fp; 
        i_fp = fopen(i_file_path, "w");
        if (i_fp == NULL) {
            g_print("Could not open intermediate file.");
            return -1;
        }

        GSList *node = term_doc_list;

        while ((node = node->next) != NULL) {
            term_docs_t* term_doc = (term_docs_t*) node->data; 
            fprintf(i_fp, "%d|", term_doc->term_id);
            GSList *next = term_doc->doc_ids;
            fprintf(i_fp, "%d", GPOINTER_TO_INT(next->data));

            while ((next = next->next) != NULL) {
                fprintf(i_fp, ",%d", GPOINTER_TO_INT(next->data));
            }
            fprintf(i_fp, "\n");
        }

        fclose(i_fp);

        g_print("\tCleanup Phase\n");
        for_each_list_item(term_doc_list, clear_term_docs);
        g_slist_free(term_doc_list);
    } 
    
    g_print("Merge Phase\n");
    // Open all intermediate files for reading.
    FILE* block_intermediate_fp[BLOCKS];
    for (int i = 0; i <= BLOCKS; i++) {
        char i_file_path[MAX_DOC_TITLE_LENGTH];
        sprintf(i_file_path, "./intermediates/%d.intermediate", i);

        block_intermediate_fp[i] = fopen(i_file_path, "r");
        if (block_intermediate_fp[i] == NULL) {
            g_print("Could not open intermediate file.");
            return -1;
        }
    }

    // Open index file for writing.
    FILE* index_fp;
    index_fp = fopen("posting.dict", "w");
    if (index_fp == NULL) {
        g_print("Could not open index file for writing.");
        return -1;
    }

    GSList *postings;
    postings = NULL;
    postings = g_slist_alloc();
    do {
        for_each_list_item(postings, clear_term_docs);
        g_slist_free(postings);
        postings = NULL;
        postings = g_slist_alloc();

        // Peek all blocks
        GHashTable* t_doc_block_map = g_hash_table_new_full(g_direct_hash, 
                g_direct_equal, NULL, (GDestroyNotify)g_slist_free);
        for (int i = 0; i <= BLOCKS; i++) {
            if (block_intermediate_fp[i] != NULL) {
                term_docs_t* t_doc = deserialize_term_doc(block_intermediate_fp[i], 1); 
                if (t_doc != NULL) {
                    postings = g_slist_insert(postings, t_doc, 1);

                    // Store the term doc to block mapping to only read from appropriate files.
                    GSList *blocks;
                    blocks = NULL;
                    if ((blocks = g_hash_table_lookup(t_doc_block_map, 
                                GINT_TO_POINTER(t_doc->term_id))) == NULL) {
                        blocks = g_slist_alloc();
                        blocks = g_slist_insert(blocks, block_intermediate_fp[i], 1);
                        g_hash_table_insert(t_doc_block_map, GINT_TO_POINTER(t_doc->term_id),
                            blocks);
                    }
                    else {
                        blocks = g_slist_insert(blocks, block_intermediate_fp[i], 1);
                    }
                }
            }
        }
        if (g_slist_length(postings) == 1) {
            g_print("No more postings lists to merge\n");
            break;
        }
        postings = g_slist_sort(postings, (GCompareFunc)term_sort_comparator);

        int smallest_term_id = ((term_docs_t*)(g_slist_nth(postings, 1)->data))->term_id;
        
        // We have the smallest term id now. So let's get all term_docs without peeking.
        for_each_list_item(postings, clear_term_docs);
        g_slist_free(postings);
        postings = NULL;
        postings = g_slist_alloc();

        GSList *blocks = (GSList*)g_hash_table_lookup(t_doc_block_map, 
                GINT_TO_POINTER(smallest_term_id));

        while ((blocks = blocks->next) != NULL) {
            FILE* block_file = (FILE*)blocks->data;
            if (block_file != NULL) {
                term_docs_t* t_doc = deserialize_term_doc(block_file, 0); 
                if (t_doc != NULL) {
                    postings = g_slist_insert(postings, t_doc, 1);
                }
            }
        }

        // We have our terms without peeking.
        for_each_list_item(postings, collect_term_docs);

        GSList *node = postings;
        while ((node = node->next) != NULL) {
            term_docs_t* term_doc = (term_docs_t*) node->data; 
            if (term_doc == NULL) {
                continue;
            }
            if (g_hash_table_lookup(id_to_term_map, GINT_TO_POINTER(term_doc->term_id)) == NULL) {
                break;
            }
            fprintf(index_fp, "%d|", term_doc->term_id);
            GSList *next = term_doc->doc_ids;
            if (next == NULL) {
                g_print("next was null\n");
                break;
            }
            fprintf(index_fp, "%d", GPOINTER_TO_INT(next->data));

            while ((next = next->next) != NULL) {
                fprintf(index_fp, ",%d", GPOINTER_TO_INT(next->data));
            }
            fprintf(index_fp, "\n");
        }

        g_hash_table_destroy(t_doc_block_map);

    } while (g_slist_length(postings) > 1);
        
    // Clean up files.
    for (int i = 0; i <= BLOCKS; i++) {
        if (block_intermediate_fp[i] != NULL) {
            fclose(block_intermediate_fp[i]);
        }
    }
    fclose(index_fp);

    FILE* term_fp;
    term_fp = fopen("term.dict", "w");
    if (term_fp == NULL) {
        g_print("Could not open term file for writing.");
        return -1;
    }

    FILE* doc_fp;
    doc_fp = fopen("doc.dict", "w");
    if (doc_fp == NULL) {
        g_print("Could not open docs file for writing.");
        return -1;
    }

    g_hash_table_foreach(id_to_term_map, (GHFunc)write_mapping, term_fp);
    g_hash_table_foreach(doc_map, (GHFunc)write_mapping, doc_fp);

    fclose(term_fp);
    fclose(doc_fp);

    g_hash_table_destroy(doc_map);
    g_hash_table_destroy(id_to_term_map);
    g_hash_table_destroy(term_to_id_map);

    return 0;
}

term_docs_t*
deserialize_term_doc(FILE* file, bool peek) {
    int c;
    int state = 0;
    long int peek_counter = 0;

    if (peek) {
        peek_counter = ftell(file);
    }

    term_docs_t *t_doc = malloc(sizeof(term_docs_t));
    t_doc->doc_ids = NULL;

    GString* temp = g_string_new("");
    while ((c = getc(file)) != EOF) {

        if (c == '\n') {
            int result = 0;
            result = atoi(temp->str);
            g_string_free(temp, TRUE);
            t_doc->doc_ids = g_slist_append(t_doc->doc_ids, GINT_TO_POINTER(result));
            break;
        }
        // Try to build a postings object
        if (state == 0) {
            // When state is 0, we are reading the term id in.
            if (c != '|') {
                g_string_append_c(temp, c);
            }
            else {
                // End of this state.
                t_doc->term_id = atoi(temp->str);
                g_string_free(temp, TRUE);
                temp = g_string_new("");
                state++;

                if (peek) {
                    // If we are peeking we don't need the associated doc list.
                    break;
                }
            }
        }
        else if (state == 1) {
            if (c != ',') {
                g_string_append_c(temp, c);
            }
            else {
                int result = 0;
                result = atoi(temp->str);
                g_string_free(temp, TRUE);
                temp = g_string_new("");
                t_doc->doc_ids = g_slist_append(t_doc->doc_ids, GINT_TO_POINTER(result));
            }
        }
    }
    
    if (peek) {
        fseek(file, peek_counter, SEEK_SET);
    }

    if (g_hash_table_lookup(id_to_term_map, GINT_TO_POINTER(t_doc->term_id)) == NULL) {
        g_slist_free(t_doc->doc_ids);
        g_free(t_doc);
        t_doc = NULL;
    }

    if (c == EOF && !peek) {
        // Mark the file as unsafe for future reading
        fclose(file);
        file = NULL;
        g_print("Block finished processing.\n");
    }

    return t_doc;
}

void
write_mapping(gpointer key, gpointer value, gpointer file) {
    FILE *fp = (FILE*)file;
    int termId = GPOINTER_TO_INT(key);
    char *termValue = (char*)value;
    fprintf(fp, "%d|%s\n", termId, termValue);
}

void
read_doc_file(int doc_id, int* term_id_counter, char* path, GSList *list) {
    int c;
    FILE *file;
    file = fopen(path, "r");
    GString *term = NULL;
    term = g_string_new("");
    while ((c = getc(file)) != EOF) {
        if (c == ' ') {
            int term_id = generate_term_mapping(term->str, term_id_counter); 
            list = g_slist_insert(list, generate_term_doc(term_id, doc_id), 1);
            g_string_free(term, TRUE);
            term = g_string_new("");
            continue;
        }
        g_string_append_c(term, (char)c);
    }
    
    int term_id = generate_term_mapping(term->str, term_id_counter); 
    list = g_slist_insert(list, generate_term_doc(term_id, doc_id), 1);
    g_string_free(term, TRUE);

    fclose(file);
}

int
generate_term_mapping(char* term, int* term_id_counter) {
    char *cleaned_term = g_strstrip(term);
    int term_id = GPOINTER_TO_INT(g_hash_table_lookup(term_to_id_map, cleaned_term));
    if (term_id > 0) {
        return GPOINTER_TO_INT(term_id);
    }
    (*term_id_counter)++;
    term_id = *term_id_counter;
    char *term_ptr = g_strdup(cleaned_term);
    g_hash_table_insert(id_to_term_map, GINT_TO_POINTER(term_id), term_ptr);
    g_hash_table_insert(term_to_id_map, term_ptr, GINT_TO_POINTER(term_id));
    return term_id;
}

term_docs_t*
generate_term_doc(int term_id, int doc_id) {
    term_docs_t *t_doc = malloc(sizeof(term_docs_t));
    t_doc->term_id = term_id;
    t_doc->doc_ids = NULL;
    t_doc->doc_ids = g_slist_append(t_doc->doc_ids, GINT_TO_POINTER(doc_id));
    return t_doc;
}

gint
key_term_comparator(char* key, term_docs_t* term_doc) {
    return g_ascii_strcasecmp(key, (char *)g_hash_table_lookup(id_to_term_map, &term_doc->term_id));
}

gint
term_sort_comparator(gconstpointer item1, gconstpointer item2) {
    term_docs_t* term_doc_1 = (term_docs_t*) item1; 
    term_docs_t* term_doc_2 = (term_docs_t*) item2; 
    if (term_doc_1 != NULL && term_doc_2 != NULL) {
        // CASE: Normal case
        char* term1 = (char*)g_hash_table_lookup(id_to_term_map, GINT_TO_POINTER(term_doc_1->term_id));
        char* term2 = (char*)g_hash_table_lookup(id_to_term_map, GINT_TO_POINTER(term_doc_2->term_id));

        if (term1 != NULL && term2 != NULL) {
            return g_ascii_strcasecmp(term1, term2);
        }
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
    if (term_doc != NULL) {
        g_print("term id = %d, term = %s, doc = ", term_doc->term_id, (char *)g_hash_table_lookup(id_to_term_map, GINT_TO_POINTER(term_doc->term_id)));
        GSList *next = term_doc->doc_ids;
        if (next != NULL) {
            g_print("%d ", GPOINTER_TO_INT(next->data));
            while ((next = next->next) != NULL) {
                g_print("%d ", GPOINTER_TO_INT(next->data));
            }
        }
        else {
            g_print("===Error===\n");
            g_print("An error was found in term id %d\n", term_doc->term_id);
            g_print("There is no doc_ids associated with it for some reason\n");
        }
        g_print("\n");
    }
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
    g_slist_free(term_docs->doc_ids);
    g_free(term_docs);
}
