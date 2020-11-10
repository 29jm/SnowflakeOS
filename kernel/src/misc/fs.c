#include <kernel/fs.h>
#include <kernel/ext2.h>
#include <kernel/proc.h>
#include <kernel/sys.h>

#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <stdio.h>
#include <math.h>

typedef struct tnode_t {
    char* name;
    inode_t* inode;
} tnode_t;

char* dirname(const char* p);
char* basename(const char* p);
void fs_print_open_fds();
void fs_cache_entries(folder_inode_t* dir_ino);

static tnode_t* root;

void init_fs() {
    root = kmalloc(sizeof(tnode_t));
    root->name = "/";
    root->inode = ext2_get_fs_inode(EXT2_ROOT_INODE);
}

void delete_tnode(tnode_t* tn) {
    inode_t* in = tn->inode;

    if (in->type == DENT_DIRECTORY) {
        folder_inode_t* ind = (folder_inode_t*) in;

        while (!list_empty(&ind->subfiles)) {
            tnode_t* subtn = list_first_entry(&ind->subfiles, tnode_t);
            delete_tnode(subtn);
            list_del(list_first(&ind->subfiles));
        }

        while (!list_empty(&ind->subfolders)) {
            tnode_t* subtn = list_first_entry(&ind->subfolders, tnode_t);
            delete_tnode(subtn);
            list_del(list_first(&ind->subfolders));
        }
    }

    kfree(in);
    kfree(tn->name);
    kfree(tn);
}

/* Builds one level of vfs nodes with the children of the given inode.
 * Do not call twice on the same inode, unless previous entries have been
 * cleared previously.
 */
void fs_cache_entries(folder_inode_t* inode) {
    sos_directory_entry_t* dent = NULL;
    uint32_t offset = 0;

    while ((dent = ext2_readdir(inode->ino.inode_no, offset)) != NULL && dent->type != DENT_INVALID) {
        offset += dent->entry_size;
        tnode_t* tn = kmalloc(sizeof(tnode_t));
        tn->inode = ext2_get_fs_inode(dent->inode);
        tn->name = strndup(dent->name, dent->name_len_low);
        list_add(dent->type == DENT_FILE ? &inode->subfiles : &inode->subfolders, tn);
        kfree(dent);
    }

    inode->dirty = false;
}

/* Returns an inode_t* from a path.
 * `flags` can be one of:
 *  - O_CREAT: create the last component of `path`
 *  - O_CREATD: same, but as a directory
 * TODO: prevent creating duplicate entries.
 */
inode_t* fs_open(const char* path, uint32_t flags) {
    char* npath = fs_normalize_path(path);

    if (!strcmp(npath, "/")) {
        kfree(npath);
        return root->inode;
    }

    tnode_t* tnode = root;
    tnode_t* prev_tnode = NULL;
    char* part = npath;
    uint32_t part_len = 0;
    bool last_part = false;

    while (!last_part && tnode != prev_tnode) {
        folder_inode_t* inode = (folder_inode_t*) tnode->inode;
        prev_tnode = tnode;
        part += part_len + 1; // Skip the separator
        part_len = strchrnul(part, '/') - part;
        last_part = part[part_len] == '\0';

        // File creation requested: now's the time
        if (last_part && (flags & O_CREAT || flags & O_CREATD)) {
            uint32_t new_ino = ext2_create(part,
                flags & O_CREAT ? DENT_FILE : DENT_DIRECTORY,
                inode->ino.inode_no);

            tnode_t* new_tn = kmalloc(sizeof(tnode_t));
            new_tn->inode = ext2_get_fs_inode(new_ino);
            new_tn->name = strdup(part);
            list_add(&inode->subfiles, new_tn);
        }

        // Build the cache if needed
        if (inode->dirty) {
            fs_cache_entries(inode);
        }

        // Search the cache, starting with subfolders
        tnode_t* ent;
        list_for_each_entry(ent, &inode->subfolders) {
            if (strlen(ent->name) == part_len &&
                    !strncmp(ent->name, part, part_len)) {
                tnode = ent;
                break;
            }
        }

        if (tnode != prev_tnode) {
            continue;
        }

        // Not a subfolder: check the subfiles
        list_for_each_entry(ent, &inode->subfiles) {
            if (strlen(ent->name) == part_len &&
                    !strncmp(ent->name, part, part_len)) {
                tnode = ent;
                break;
            }
        }
    }

    kfree(npath);
    return tnode != prev_tnode ? tnode->inode : NULL;
}

/* From an inode number, finds the corresponding inode_t*.
 */
inode_t* fs_find_inode(folder_inode_t* parent, uint32_t inode) {
    tnode_t* tn;
    folder_inode_t* fin;

    list_for_each_entry(tn, &parent->subfiles) {
        if (tn->inode->inode_no == inode) {
            return tn->inode;
        }
    }

    list_for_each_entry(fin, &parent->subfolders) {
        if (tn->inode->inode_no == inode) {
            return tn->inode;
        }

        inode_t* subresult = fs_find_inode(fin, inode);

        if (subresult) {
            return subresult;
        }
    }

    return NULL;
}

uint32_t fs_mkdir(const char* path, uint32_t mode) {
    UNUSED(mode);

    // Fail if the path exists already
    if (fs_open(path, O_RDONLY)) {
        return FS_INVALID_INODE;
    }

    inode_t* in = fs_open(path, O_CREATD);

    if (!in) {
        return FS_INVALID_INODE;
    }

    return in->inode_no;
}

/* A process has released its grip on a file: TODO something.
 */
void fs_close(uint32_t inode) {
    UNUSED(inode);
}

uint32_t fs_read(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size) {
    return ext2_read(inode, offset, buf, size);
}

uint32_t fs_write(uint32_t inode, uint8_t* buf, uint32_t size) {
    inode_t* in = fs_find_inode((folder_inode_t*) root->inode, inode);

    if (!in) {
        return 0;
    }

    uint32_t written = ext2_append(inode, buf, size);
    in->size += written;

    return written;
}

uint32_t fs_readdir(uint32_t inode, uint32_t offset, sos_directory_entry_t* d_ent, uint32_t size) {
    sos_directory_entry_t* ent = ext2_readdir(inode, offset);

    // TODO: check if empty entries aren't actually valid
    if (ent == NULL || ent->name[0] == '\0') {
        return 0;
    }

    uint32_t unpadded_size = min(ent->entry_size, 263);

    if (unpadded_size < size) {
        memcpy(d_ent, ent, unpadded_size);
        d_ent->name[d_ent->name_len_low] = '\0';
    } else {
        kfree(ent);
        return 0;
    }

    kfree(ent);

    return d_ent->entry_size;
}

/* Returns the absolute version of `p`, free of oddities,
 * dynamically allocated.
 * TODO: make it use static memory.
 */
char* fs_normalize_path(const char* p) {
    char* np = kmalloc(MAX_PATH);
    strcpy(np, p);

    if (!strcmp(np, "/")) {
        return np;
    }

    if (!strcmp(np, ".")) {
        return proc_get_cwd();
    }

    // Make the path absolute
    if (p[0] != '/') {
        char* cwd = proc_get_cwd();
        strcpy(np, cwd);
        strcat(np, "/");
        strcat(np, p);
        kfree(cwd);
    }

    // Trim trailing slashes
    while (np[strlen(np) - 1] == '/') {
        np[strlen(np) - 1] = '\0';
    }

    // Trim '//' sequences
    char* s = NULL;
    while ((s = strstr(np, "//"))) {
        uint32_t n = 0;

        // Make sure to keep exaclty one /
        if (strstr(np, "///") != s) {
            n = 1;
        } else {
            n = 2;
        }

        char* npp = kmalloc(MAX_PATH);
        strncpy(npp, np, s - np);
        npp[s - np] = '\0';
        strcat(npp, s + n);
        kfree(np);
        np = npp;
    }

    return np;
}

/* Returns the path to the parent directory of the thing pointed to by `p`,
 * statically allocated.
 * Expects `p` to be a normalized path.
 * TODO: repurpose those functions in libc.
 */
char* dirname(const char* p) {
    static char dp[MAX_PATH] = "";
    char* last_sep = strrchr(p, '/');

    strcpy(dp, p);

    if (!strcmp(dp, "/")) {
        return dp;
    }

    if (last_sep == p) {
        dp[1] = '\0';
    } else {
        dp[last_sep - p] = '\0';
    }

    return dp;
}

/* Returns the name of the file pointed to by `p`.
 */
char* basename(const char* p) {
    char* last_sep = strrchr(p, '/');

    if (!strcmp(p, "/")) {
        return (char*) p;
    }

    return last_sep + 1;
}