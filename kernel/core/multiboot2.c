/*
 * multiboot2.c - Multiboot2 tag list walker
 */

#include "multiboot2.h"

void mb2_walk_tags(u64 mb2_info_addr, mb2_tag_visitor_t visit, void *ctx)
{
    struct mb2_info_header *header = (struct mb2_info_header *)mb2_info_addr;
    u8 *ptr = (u8 *)mb2_info_addr + sizeof(struct mb2_info_header);
    u8 *end = (u8 *)mb2_info_addr + header->total_size;

    while (ptr < end) {
        struct mb2_tag *tag = (struct mb2_tag *)ptr;

        if (tag->type == MB2_TAG_TYPE_END) {
            break;
        }

        if (!visit(tag, ctx)) {
            return;
        }

        /* Tags are 8-byte aligned; size does not itself include
         * this padding, so round up before advancing. */
        u32 advance = (tag->size + 7) & ~(u32)7;
        ptr += advance;
    }
}
