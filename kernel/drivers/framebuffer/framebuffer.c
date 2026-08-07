#include "drivers/framebuffer.h"
#include "multiboot2.h"
#include "printk.h"

struct mb2_tag_framebuffer {
    u32 type;
    u32 size;
    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u8 framebuffer_bpp;
    u8 type_field;
    u16 reserved;
    u32 red_field_position;
    u32 red_mask_size;
    u32 green_field_position;
    u32 green_mask_size;
    u32 blue_field_position;
    u32 blue_mask_size;
    u32 reserved2;
} __attribute__((packed));

static struct framebuffer_info fb_info;

static bool framebuffer_tag_visitor(struct mb2_tag *tag, void *ctx)
{
    (void)ctx;
    if (tag->type != MB2_TAG_TYPE_FRAMEBUFFER) {
        return true;
    }

    struct mb2_tag_framebuffer *fb = (struct mb2_tag_framebuffer *)tag;
    fb_info.address = (void *)(uptr)fb->framebuffer_addr;
    fb_info.pitch = fb->framebuffer_pitch;
    fb_info.width = fb->framebuffer_width;
    fb_info.height = fb->framebuffer_height;
    fb_info.bpp = fb->framebuffer_bpp;
    fb_info.framebuffer_type = fb->type_field;
    fb_info.red_field_position = (u8)fb->red_field_position;
    fb_info.red_mask_size = (u8)fb->red_mask_size;
    fb_info.green_field_position = (u8)fb->green_field_position;
    fb_info.green_mask_size = (u8)fb->green_mask_size;
    fb_info.blue_field_position = (u8)fb->blue_field_position;
    fb_info.blue_mask_size = (u8)fb->blue_mask_size;
    fb_info.available = (fb_info.framebuffer_type == 1 &&
                         (fb_info.bpp == 24 || fb_info.bpp == 32));

    if (!fb_info.available) {
        printk_level(LOG_WARN,
                     "framebuffer: unsupported format type=%u bpp=%u, ignoring\n",
                     fb_info.framebuffer_type, fb_info.bpp);
        return true;
    }

    printk_level(LOG_INFO,
                 "framebuffer: detected %ux%u %u-bit buffer, pitch=%u\n",
                 fb_info.width, fb_info.height, fb_info.bpp, fb_info.pitch);

    return false; 
}

void framebuffer_init(u64 mb2_info_addr)
{
    fb_info.address = NULL;
    fb_info.pitch = 0;
    fb_info.width = 0;
    fb_info.height = 0;
    fb_info.bpp = 0;
    fb_info.framebuffer_type = 0;
    fb_info.red_field_position = 0;
    fb_info.red_mask_size = 0;
    fb_info.green_field_position = 0;
    fb_info.green_mask_size = 0;
    fb_info.blue_field_position = 0;
    fb_info.blue_mask_size = 0;
    fb_info.available = false;

    mb2_walk_tags(mb2_info_addr, framebuffer_tag_visitor, NULL);
}

bool framebuffer_available(void)
{
    return fb_info.available;
}

const struct framebuffer_info *framebuffer_info(void)
{
    return &fb_info;
}

static inline u32 framebuffer_pack_color(u8 r, u8 g, u8 b)
{
    return FRAMEBUFFER_RGB(r, g, b);
}

void framebuffer_clear(u32 color)
{
    if (!fb_info.available) {
        return;
    }

    u8 *pixels = (u8 *)fb_info.address;
    u32 bytes_per_pixel = fb_info.bpp / 8;
    u32 row_bytes = fb_info.pitch;

    for (u32 y = 0; y < fb_info.height; y++) {
        u8 *row = pixels + (u64)y * row_bytes;
        for (u32 x = 0; x < fb_info.width; x++) {
            u8 *pixel = row + (u64)x * bytes_per_pixel;
            if (bytes_per_pixel == 4) {
                pixel[0] = (u8)(color & 0xFF);
                pixel[1] = (u8)((color >> 8) & 0xFF);
                pixel[2] = (u8)((color >> 16) & 0xFF);
                pixel[3] = 0;
            } else if (bytes_per_pixel == 3) {
                pixel[0] = (u8)(color & 0xFF);
                pixel[1] = (u8)((color >> 8) & 0xFF);
                pixel[2] = (u8)((color >> 16) & 0xFF);
            }
        }
    }
}

void framebuffer_put_pixel(u32 x, u32 y, u32 color)
{
    if (!fb_info.available || x >= fb_info.width || y >= fb_info.height) {
        return;
    }

    u8 *base = (u8 *)fb_info.address + (u64)y * fb_info.pitch;
    u32 bytes_per_pixel = fb_info.bpp / 8;
    u8 *pixel = base + (u64)x * bytes_per_pixel;

    if (bytes_per_pixel == 4) {
        pixel[0] = (u8)(color & 0xFF);
        pixel[1] = (u8)((color >> 8) & 0xFF);
        pixel[2] = (u8)((color >> 16) & 0xFF);
        pixel[3] = 0;
    } else if (bytes_per_pixel == 3) {
        pixel[0] = (u8)(color & 0xFF);
        pixel[1] = (u8)((color >> 8) & 0xFF);
        pixel[2] = (u8)((color >> 16) & 0xFF);
    }
}

void framebuffer_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color)
{
    for (u32 row = y; row < y + height && row < fb_info.height; row++) {
        for (u32 col = x; col < x + width && col < fb_info.width; col++) {
            framebuffer_put_pixel(col, row, color);
        }
    }
}

void framebuffer_draw_gradient(void)
{
    if (!fb_info.available) {
        return;
    }

    for (u32 y = 0; y < fb_info.height; y++) {
        u8 r = (u8)((y * 255) / fb_info.height);
        for (u32 x = 0; x < fb_info.width; x++) {
            u8 g = (u8)((x * 255) / fb_info.width);
            framebuffer_put_pixel(x, y, framebuffer_pack_color(r, g, 0x80));
        }
    }
}
