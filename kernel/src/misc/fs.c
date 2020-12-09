#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/sys.h>

#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <stdio.h>
#include <math.h>

#define FS(inode) ((inode_t*) inode)->fs

typedef struct tnode_t {
    char* name;
    inode_t* inode;
} tnode_t;

char* dirname(const char* p);
char* basename(const char* p);
uint32_t tnode_to_directory_entry(tnode_t* tn, sos_directory_entry_t* d_ent, uint32_t size);
void fs_build_tree_level(folder_inode_t* dir_ino, inode_t* parent);

static tnode_t* root;

void init_fs(fs_t* fs) {
    fs_mount("/", fs);
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
void fs_build_tree_level(folder_inode_t* inode, inode_t* parent) {
    sos_directory_entry_t* dent = NULL;
    uint32_t offset = 0;

    /* Add "." and ".." ourselves, don't trust the fs */
    tnode_t* tn = kmalloc(sizeof(tnode_t));
    tn->inode = (inode_t*) inode;
    tn->name = strdup(".");
    list_add(&inode->subfolders, tn);

    tn = kmalloc(sizeof(tnode_t));
    tn->inode = parent;
    tn->name = strdup("..");
    list_add(&inode->subfolders, tn);

    /* Add the rest of the entries */
    while ((dent = FS(inode)->readdir(FS(inode), inode->ino.inode_no, offset)) != NULL && dent->type != DENT_INVALID) {
        offset += dent->entry_size;

        if (strncmp(dent->name, ".", dent->name_len_low) && strncmp(dent->name, "..", dent->name_len_low)) {
            tn = kmalloc(sizeof(tnode_t));
            tn->name = strndup(dent->name, dent->name_len_low);
            tn->inode = FS(inode)->get_fs_inode(FS(inode), dent->inode);
            list_add(dent->type == DENT_FILE ? &inode->subfiles : &inode->subfolders, tn);
        }

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
            uint32_t new_ino = FS(inode)->create(FS(inode), part,
                flags & O_CREAT ? DENT_FILE : DENT_DIRECTORY,
                inode->ino.inode_no);

            tnode_t* new_tn = kmalloc(sizeof(tnode_t));
            new_tn->inode = FS(inode)->get_fs_inode(FS(inode), new_ino);
            new_tn->name = strdup(part);
            list_add(flags & O_CREAT ? &inode->subfiles : &inode->subfolders, new_tn);
        }

        // Build the tree as needed
        if (inode->dirty) {
            fs_build_tree_level(inode, prev_tnode->inode);
        }

        // Search the tree, starting with subfolders
        tnode_t* ent;
        list_for_each_entry(ent, &inode->subfolders) {
            if (strlen(ent->name) == part_len &&
                    !strncmp(ent->name, part, part_len)) {
                tnode = ent;

                if (((folder_inode_t*) tnode->inode)->dirty) {
                    fs_build_tree_level((folder_inode_t*) tnode->inode, prev_tnode->inode);
                }

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

/* Mounts a filesystem at the given path in the existing VFS.
 * The first filesystem can be mounted at "/".
 */
void fs_mount(const char* mount_point, fs_t* fs) {
    /* Special case for the first filesystem mounted */
    if (!root && !strcmp(mount_point, "/")) {
        root = kmalloc(sizeof(tnode_t));
        root->inode = (inode_t*) fs->root;
        root->name = strdup("/");
        return;
    }

    /* Check the mount point's validity */
    folder_inode_t* mnt_in = (folder_inode_t*) fs_open(mount_point, O_RDONLY);

    if (mnt_in->ino.type != DENT_DIRECTORY) {
        printke("mount: mountpoint not a directory");
        return;
    }

    uint32_t num_children = 0;
    tnode_t* child;
    list_for_each_entry(child, &mnt_in->subfolders) {
        num_children++;
    }

    if (num_children > 2 || !list_empty(&mnt_in->subfiles)) {
        printke("mount: mountpoint not empty");
        return;
    }

    /* Empty its "." and ".." entries */
    while (!list_empty(&mnt_in->subfolders)) {
        tnode_t* tn = list_first_entry(&mnt_in->subfolders, tnode_t);
        kfree(tn->name);
        kfree(tn);
        list_del(list_first(&mnt_in->subfolders));
    }

    // TODO: make umount possible
    mnt_in->ino = fs->root->ino;
    mnt_in->dirty = true;
    mnt_in->subfiles = LIST_HEAD_INIT(mnt_in->subfiles);
    mnt_in->subfolders = LIST_HEAD_INIT(mnt_in->subfolders);
}

uint32_t fs_mkdir(const char* path, uint32_t mode) {
    UNUSED(mode);

    // Fail if the path exists already
    if (fs_open(path, O_RDONLY)) {
        return FS_INVALID_INODE;
    }

    inode_t* in = fs_open(path, O_CREATD);

    return in ? in->inode_no : FS_INVALID_INODE;
}

int32_t fs_unlink(const char* path) {
    /* Check that we're unlinking a file */
    char* npath = fs_normalize_path(path);
    folder_inode_t* d_in = (folder_inode_t*) fs_open(dirname(npath), O_RDONLY);
    inode_t* in = fs_open(npath, O_RDONLY);
    kfree(npath);

    if (!d_in || !in || in->type != DENT_FILE) {
        return -1;
    }

    /* Ask the filesystem to unlink that inode */
    int32_t ret = FS(d_in)->unlink(FS(d_in), d_in->ino.inode_no, in->inode_no);

    if (ret == -1) {
        return -1;
    }

    /* Prune it from the tree */
    list_t* iter;
    tnode_t* tn;
    list_for_each(iter, tn, &d_in->subfiles) {
        if (tn->inode->inode_no == in->inode_no) {
            kfree(tn->name);
            kfree(tn);

            if (--in->hardlinks == 0) {
                kfree(in);
            }

            list_del(iter);
            break;
        }
    }

    return 0;
}

/* Renames the file pointed to by `oldp` to `newp`, moving it across directories
 * as needed. See "man 2 rename" for the expected behavior.
 * Note: doesn't support renaming a directory to an existing empty directory.
 */
int32_t fs_rename(const char* oldp, const char* newp) {
    inode_t* old = fs_open(oldp, O_RDONLY);
    inode_t* new  = fs_open(newp, O_RDONLY);

    if (!old) {
        return -1;
    }

    if (old->type == DENT_DIRECTORY && new) {
        return -1;
    }

    /* Both point to the same file */
    if (new == old) {
        return 0;
    }

    /* Moving across filesystems is unsupported right now */
    if (new && new->fs != old->fs) {
        return -1;
    }

    /* Destination will be replaced, remove it */
    if (new) {
        if (fs_unlink(newp) == -1) {
            return -1;
        }
    }

    /* Do the renaming on the fs */
    char* noldp = fs_normalize_path(oldp);
    char* nnewp = fs_normalize_path(newp);
    folder_inode_t* src = (folder_inode_t*) fs_open(dirname(noldp), O_RDONLY);
    folder_inode_t* dst = (folder_inode_t*) fs_open(dirname(nnewp), O_RDONLY);

    int32_t ret = FS(old)->rename(FS(old), src->ino.inode_no, old->inode_no, dst->ino.inode_no);

    if (ret == -1) {
        kfree(noldp);
        kfree(nnewp);
        return -1;
    }

    /* Rename in VFS too: remove from the original parent directory */
    list_t* to_iterate = old->type == DENT_DIRECTORY ?
        &src->subfolders : &src->subfiles;
    list_t* iter;
    tnode_t* tn;
    list_for_each(iter, tn, to_iterate) {
        if (tn->inode->inode_no == old->inode_no) {
            list_del(iter);
            break;
        }
    }

    /* Add to the destination parent directory */
    kfree(tn->name);
    tn->name = strdup(basename(nnewp));
    list_t* to_add_to = old->type == DENT_DIRECTORY ?
        &dst->subfolders : &dst->subfiles;
    list_add(to_add_to, tn);

    kfree(noldp);
    kfree(nnewp);

    return 0;
}

int32_t fs_stat(const char* path, stat_t* buf) {
    inode_t* in = fs_open(path, O_RDONLY);

    if (!in) {
        return -1;
    }

    return FS(in)->stat(FS(in), in->inode_no, buf);
}

/* A process has released its grip on a file: notify the fs.
 */
int32_t fs_close(inode_t* in) {
    return FS(in)->close(FS(in), in->inode_no);
}

uint32_t fs_read(inode_t* in, uint32_t offset, uint8_t* buf, uint32_t size) {
    return FS(in)->read(FS(in), in->inode_no, offset, buf, size);
}

uint32_t fs_write(inode_t* in, uint8_t* buf, uint32_t size) {
    if (!in) {
        return 0;
    }

    uint32_t written = FS(in)->append(FS(in), in->inode_no, buf, size);
    in->size += written;

    return written;
}

uint32_t fs_readdir(inode_t* in, uint32_t index, sos_directory_entry_t* d_ent, uint32_t size) {
    if (in->type != DENT_DIRECTORY) {
        printke("not a directory");
        return 0;
    }

    folder_inode_t* fin = (folder_inode_t*) in;

    if (fin->dirty) {
        printke("dirty inode being readdir'ed");
        return 0;
    }

    uint32_t i = 0;
    tnode_t* tn;
    list_for_each_entry(tn, &fin->subfolders) {
        if (i++ == index) {
            return tnode_to_directory_entry(tn, d_ent, size);
        }
    }

    list_for_each_entry(tn, &fin->subfiles) {
        if (i++ == index) {
            return tnode_to_directory_entry(tn, d_ent, size);
        }
    }

    return 0;
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

/* Writes the given `tnode_t` to `d_ent` with proper form, returns the entry size.
 * If `size` is too small, does nothing and returns 0.
 */
uint32_t tnode_to_directory_entry(tnode_t* tn, sos_directory_entry_t* d_ent, uint32_t size) {
    uint32_t esize = sizeof(sos_directory_entry_t) + strlen(tn->name) + 1;

    if (size < esize) {
        return 0;
    }

    d_ent->inode = tn->inode->inode_no;
    strcpy(d_ent->name, tn->name);
    d_ent->name_len_low = strlen(tn->name);
    d_ent->type = tn->inode->type;
    d_ent->entry_size = esize;

    return esize;
}
