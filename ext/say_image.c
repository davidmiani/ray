#include "say.h"

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

static GLuint say_current_texture = 0;
static say_context *say_texture_last_context = NULL;

static void say_texture_make_current(GLuint texture) {
  say_context *context = say_context_current();

  if (texture != say_current_texture ||
      context != say_texture_last_context) {
    say_texture_last_context = context;
    say_current_texture = texture;

    glBindTexture(GL_TEXTURE_2D, texture);
  }
}

static void say_texture_will_delete(GLuint texture) {
  if (say_current_texture == texture)
    say_current_texture = 0;
}

say_image *say_image_create() {
  say_context_ensure();

  say_image *img = (say_image*)malloc(sizeof(say_image));

  img->texture = 0;
  glGenTextures(1, &(img->texture));

  img->pixels          = NULL;
  img->texture_updated = 1;

  img->width  = 0;
  img->height = 0;

  img->smooth = 1;
  say_image_set_smooth(img, 0);

  return img;
}

void say_image_free(say_image *img) {
  say_context_ensure();

  say_texture_will_delete(img->texture);
  glDeleteTextures(1, &(img->texture));

  if (img->pixels)
    free(img->pixels);

  free(img);
}

bool say_image_load_raw(say_image *img, size_t width, size_t height,
                        say_color *pixels) {
  if (!say_image_create_with_size(img, width, height))
    return false;

  memcpy(img->pixels, pixels, sizeof(say_color) * width * height);
  return true;
}

bool say_image_load_file(say_image *img, const char *filename) {
  int width, height, comp = 4;

  stbi_uc *buf = stbi_load(filename, &width, &height, &comp, 4);
  if (!buf)
    return false;

  if (!say_image_load_raw(img, width, height, (say_color*)buf))
    return false;
  stbi_image_free(buf);

  return true;
}

bool say_image_load_from_memory(say_image *img, size_t size,
                                const char *buffer) {
  int width, height, comp = 4;

  stbi_uc *buf = stbi_load_from_memory((const uint8_t*)buffer, size,
                                       &width, &height, &comp, 4);
  if (!buf)
    return false;

  if (!say_image_load_raw(img, width, height, (say_color*)buf))
    return false;
  stbi_image_free(buf);

  return true;
}

bool say_image_create_with_size(say_image *img, size_t w, size_t h) {
  if (w == 0 || h == 0) {
    say_error_set("can't create empty image");
    return false;
  }

  if (img->width != w || img->height != h) {
    if (img->pixels) free(img->pixels);
    img->pixels = malloc(sizeof(say_color) * w * h);

    say_texture_make_current(img->texture);
    glGetError(); /* Ignore potential previous errors */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    if (glGetError()) {
      say_error_set("could not create texture");
      return false;
    }
  }

  img->width  = w;
  img->height = h;

  img->texture_updated = 0;

  return true;
}

static bool say_image_assert_non_empty(say_image *img) {
  if (img->width == 0 || img->height == 0) {
    say_error_set("can't save empty image");
    return false;
  }

  return true;
}

bool say_image_write_bmp(say_image *img, const char *filename) {
  if (!say_image_assert_non_empty(img))
    return false;

  stbi_write_bmp(filename, img->width, img->height, 4, img->pixels);

  return true;
}

bool say_image_write_png(say_image *img, const char *filename) {
  if (!say_image_assert_non_empty(img))
    return false;

  stbi_write_png(filename, img->width, img->height, 4, img->pixels, 0);

  return true;
}

bool say_image_write_tga(say_image *img, const char *filename) {
  if (!say_image_assert_non_empty(img))
    return false;

  stbi_write_tga(filename, img->width, img->height, 4, img->pixels);

  return true;
}

bool say_image_write(say_image *img, const char *filename) {
  size_t len = strlen(filename);

  if (len < 4)
    return say_image_write_bmp(img, filename);
  else {
    const char *ext = (filename + len - 4);
    if (strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0)
      return say_image_write_png(img, filename);
    else if (strcmp(ext, ".tga") == 0 || strcmp(ext, ".TGA") == 0)
      return say_image_write_tga(img, filename);
    else
      say_image_write_bmp(img, filename);
  }

  return true;
}

size_t say_image_get_width(say_image *img) {
  return img->width;
}

size_t say_image_get_height(say_image *img) {
  return img->height;
}

say_vector2 say_image_get_size(say_image *img) {
  return say_make_vector2(img->width, img->height);
}

bool say_image_resize(say_image *img, size_t w, size_t h) {
  size_t old_w = img->width, old_h = img->height;

  say_color *cpy = malloc(sizeof(say_color) * img->width * img->height);
  memcpy(cpy, img->pixels, sizeof(say_color) * img->width * img->height);

  if (!say_image_create_with_size(img, w, h))
    return false;

  size_t row_size = sizeof(say_color) * old_h;

  for (size_t y = 0; y < old_h; y++) {
    memcpy(&img->pixels[y * img->width], &cpy[y * old_h], row_size);
    for (size_t x = old_w; x < img->width; x++)
      img->pixels[y * img->height + x] = say_make_color(255, 255, 255, 0);
  }

  for (size_t y = old_h; y < img->height; y++) {
    for (size_t x = 0; x < img->width; x++)
      img->pixels[y * img->height + x] = say_make_color(255, 255, 255, 0);
  }

  free(cpy);

  return true;
}

uint8_t say_image_is_smooth(say_image *img) {
  return img->smooth;
}

void say_image_set_smooth(say_image *img, uint8_t val) {
  if (img->smooth != val) {
    img->smooth = val;

    say_context_ensure();
    say_texture_make_current(img->texture);

    GLenum interp = val ? GL_LINEAR : GL_NEAREST;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp);
  }
}

say_rect say_image_get_tex_rect(say_image *img, say_rect rect) {
  if (img->width == 0 || img->height == 0)
    return say_make_rect(0, 0, 0, 0);

  return say_make_rect(rect.x / img->width, rect.y / img->height,
                       rect.w / img->width, rect.h / img->height);
}

say_color *say_image_get_buffer(say_image *img) {
  return img->pixels;
}

say_color say_image_get(say_image *img, size_t x, size_t y) {
  return img->pixels[y * img->width + x];
}

void say_image_set(say_image *img, size_t x, size_t y, say_color color) {
  img->pixels[y * img->width + x] = color;
  img->texture_updated = 0;
}

void say_image_bind(say_image *img) {
  say_context_ensure();
  say_texture_make_current(img->texture);

  if (!img->texture_updated)
    say_image_update_texture(img);
}

void say_image_update_texture(say_image *img) {
  if (!img->pixels)
    return;

  say_texture_make_current(img->texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0,
                  0, 0,
                  img->width, img->height,
                  GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

  img->texture_updated = 1;
}

void say_image_unbind() {
  say_texture_make_current(0);
}
