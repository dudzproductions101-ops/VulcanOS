
#include "fs/vulcanfs.h"
#include "fs/vfs.h"
#include "mm/allocator.h"
#include "printk.h"

#define VULCANFS_MAX_CHILDREN 32
#define VULCANFS_INITIAL_CAPACITY 256

struct vulcanfs_node {
    struct inode *node;

    u8 *data;
    usize data_capacity;

    struct vulcanfs_node *children[VULCANFS_MAX_CHILDREN];
    char child_names[VULCANFS_MAX_CHILDREN][VULCAN_FILENAME_MAX];
    int child_count;
};

static u64 next_inode_number = 1;

static void copy_name(char *dest, const char *src)
{
    int i = 0;
    for (; i < VULCAN_FILENAME_MAX - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static bool names_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static const struct inode_ops vulcanfs_ops;

static struct inode *vulcanfs_new_inode(enum inode_type type)
{
    struct inode *node = kmalloc(sizeof(struct inode));
    struct vulcanfs_node *priv = kmalloc(sizeof(struct vulcanfs_node));

    if (!node || !priv) {
        printk_level(LOG_ERROR, "vulcanfs: out of memory creating inode\n");
        return NULL;
    }

    node->inode_number = next_inode_number++;
    node->type = type;
    node->permissions = VULCAN_PERM_READ | VULCAN_PERM_WRITE;
    if (type == INODE_DIRECTORY) {
        node->permissions |= VULCAN_PERM_EXEC;

    }
    node->size = 0;
    node->ops = &vulcanfs_ops;
    node->private_data = priv;
    node->refcount = 0;

    priv->node = node;
    priv->data = NULL;
    priv->data_capacity = 0;
    priv->child_count = 0;

    return node;
}

static isize vulcanfs_read(struct inode *node, void *buf, usize size, usize offset)
{
    struct vulcanfs_node *priv = node->private_data;

    if (node->type != INODE_FILE) {
        return VULCAN_FS_ERR_IS_DIR;
    }
    if (offset >= node->size) {
        return 0;
    }

    usize available = node->size - offset;
    usize to_read = (size < available) ? size : available;

    for (usize i = 0; i < to_read; i++) {
        ((u8 *)buf)[i] = priv->data[offset + i];
    }

    return (isize)to_read;
}

static bool vulcanfs_grow_capacity(struct vulcanfs_node *priv, usize needed)
{
    if (priv->data_capacity >= needed) {
        return true;
    }

    usize new_capacity = priv->data_capacity == 0 ? VULCANFS_INITIAL_CAPACITY : priv->data_capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    u8 *new_data = kmalloc(new_capacity);
    if (!new_data) {
        return false;
    }

    for (usize i = 0; i < priv->data_capacity; i++) {
        new_data[i] = priv->data[i];
    }
    if (priv->data) {
        kfree(priv->data);
    }

    priv->data = new_data;
    priv->data_capacity = new_capacity;
    return true;
}

static isize vulcanfs_write(struct inode *node, const void *buf, usize size, usize offset)
{
    struct vulcanfs_node *priv = node->private_data;

    if (node->type != INODE_FILE) {
        return VULCAN_FS_ERR_IS_DIR;
    }

    usize needed = offset + size;
    if (!vulcanfs_grow_capacity(priv, needed)) {
        return VULCAN_FS_ERR_NO_SPACE;
    }

    for (usize i = 0; i < size; i++) {
        priv->data[offset + i] = ((const u8 *)buf)[i];
    }

    if (needed > node->size) {
        node->size = needed;
    }

    return (isize)size;
}

static struct inode *vulcanfs_lookup(struct inode *dir, const char *name)
{
    struct vulcanfs_node *priv = dir->private_data;

    if (dir->type != INODE_DIRECTORY) {
        return NULL;
    }

    for (int i = 0; i < priv->child_count; i++) {
        if (names_equal(priv->child_names[i], name)) {
            return priv->children[i]->node;
        }
    }

    return NULL;
}

static struct inode *vulcanfs_create_child(struct inode *dir, const char *name, enum inode_type type)
{
    struct vulcanfs_node *priv = dir->private_data;

    if (dir->type != INODE_DIRECTORY) {
        return NULL;
    }
    if (priv->child_count >= VULCANFS_MAX_CHILDREN) {
        printk_level(LOG_WARN, "vulcanfs: directory full (max %d entries)\n",
                     VULCANFS_MAX_CHILDREN);
        return NULL;
    }
    if (vulcanfs_lookup(dir, name)) {
        return NULL;
    }

    struct inode *child = vulcanfs_new_inode(type);
    if (!child) {
        return NULL;
    }

    int idx = priv->child_count++;
    copy_name(priv->child_names[idx], name);
    priv->children[idx] = child->private_data;

    return child;
}

static bool vulcanfs_unlink_child(struct inode *dir, const char *name)
{
    struct vulcanfs_node *priv = dir->private_data;

    if (dir->type != INODE_DIRECTORY) {
        return false;
    }

    for (int i = 0; i < priv->child_count; i++) {
        if (!names_equal(priv->child_names[i], name)) {
            continue;
        }

        struct vulcanfs_node *target = priv->children[i];

        if (target->node->type == INODE_DIRECTORY && target->child_count > 0) {
            return false;

        }

        for (int j = i; j < priv->child_count - 1; j++) {
            priv->children[j] = priv->children[j + 1];
            copy_name(priv->child_names[j], priv->child_names[j + 1]);
        }
        priv->child_count--;

        if (target->data) {
            kfree(target->data);
        }
        kfree(target->node);
        kfree(target);

        return true;
    }

    return false;
}

static bool vulcanfs_readdir(struct inode *dir, usize index, struct dirent *out)
{
    struct vulcanfs_node *priv = dir->private_data;

    if (dir->type != INODE_DIRECTORY) {
        return false;
    }
    if (index >= (usize)priv->child_count) {
        return false;
    }

    copy_name(out->name, priv->child_names[index]);
    out->inode_number = priv->children[index]->node->inode_number;
    out->type = priv->children[index]->node->type;

    return true;
}

static const struct inode_ops vulcanfs_ops = {
    .read = vulcanfs_read,
    .write = vulcanfs_write,
    .lookup = vulcanfs_lookup,
    .create = vulcanfs_create_child,
    .unlink = vulcanfs_unlink_child,
    .readdir = vulcanfs_readdir,
};

struct inode *vulcanfs_create(void)
{
    struct inode *root = vulcanfs_new_inode(INODE_DIRECTORY);
    if (!root) {
        return NULL;
    }

    printk_level(LOG_INFO, "vulcanfs: created new instance, root inode=%llu\n",
                 root->inode_number);

    return root;
}
