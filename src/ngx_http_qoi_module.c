#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

static ngx_int_t ngx_http_qoi_handler(ngx_http_request_t *r)
{
  size_t root;
  u_char *p;
  ngx_buf_t *buf;
  ngx_chain_t out;
  ngx_http_core_loc_conf_t *clcf;
  ngx_open_file_info_t of;
  ngx_str_t dest, src;
  ngx_table_elt_t *header;

  clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

  p = ngx_http_map_uri_to_path(r, &src, &root, 0);
  if (p == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  p = ngx_http_map_uri_to_path(r, &dest, &root, sizeof(".png") - 1);
  *p++ = '.';
  *p++ = 'p';
  *p++ = 'n';
  *p++ = 'g';
  *p = '\0';
  dest.len = p - dest.data;

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "qoi: source filename: \"%s\"", src.data);
  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "qoi: destination filename: \"%s\"", dest.data);

  /* convert: qoi -> png*/
  {
    int encoded = 0;
    void *pixels = NULL;
    qoi_desc desc;

    pixels = qoi_read((const char *) src.data, &desc, 0);
    if (pixels == NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "qoi: Couldn't load/decode %s", src.data);
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    encoded = stbi_write_png((const char *) dest.data,
                             desc.width, desc.height, desc.channels,
                             pixels, 0);

    free(pixels);

    if (!encoded) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "qoi: Couldn't write/encode %s", dest.data);
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
  }

  ngx_memzero(&of, sizeof(ngx_open_file_info_t));

  of.read_ahead = clcf->read_ahead;
  of.directio = clcf->directio;
  of.valid = clcf->open_file_cache_valid;
  of.min_uses = clcf->open_file_cache_min_uses;
  of.errors = clcf->open_file_cache_errors;
  of.events = clcf->open_file_cache_events;

  if (ngx_http_set_disable_symlinks(r, clcf, &dest, &of) != NGX_OK) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  if (ngx_open_cached_file(clcf->open_file_cache, &dest, &of, r->pool)
      != NGX_OK) {
    ngx_uint_t level;
    switch (of.err) {
      case 0:
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

      case NGX_ENOENT:
      case NGX_ENOTDIR:
      case NGX_ENAMETOOLONG:
        return NGX_DECLINED;

      case NGX_EACCES:
#if (NGX_HAVE_OPENAT)
      case NGX_EMLINK:
      case NGX_ELOOP:
#endif
        level = NGX_LOG_ERR;
        break;
      default:
        level = NGX_LOG_CRIT;
        break;
    }

    ngx_log_error(level, r->connection->log, of.err,
                  "qoi: %s \"%s\" failed", of.failed, dest.data);

    return NGX_DECLINED;
  }

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "qoi: uri status: %s\n", r->uri.data);

  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_length_n = of.size;
  r->headers_out.last_modified_time = of.mtime;

  header = ngx_list_push(&r->headers_out.headers);
  header->hash = 1;
  ngx_str_set(&header->key, "Content-Type");
  ngx_str_set(&header->value, "image/png");
  r->headers_out.content_encoding = header;

  buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  buf->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
  ngx_http_send_header(r);
  buf->file_pos = 0;
  buf->file_last = of.size;

  buf->in_file = buf->file_last ? 1 : 0;
  buf->last_buf = (r == r->main) ? 1 : 0;
  buf->last_in_chain = 1;

  buf->file->fd = of.fd;
  out.buf = buf;
  out.next = NULL;

  return ngx_http_output_filter(r, &out);
}

static char *ngx_http_qoi(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_http_core_loc_conf_t *clcf;

  clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_qoi_handler;

  return NGX_CONF_OK;
}

static ngx_command_t ngx_http_qoi_commands[] = {
  {
    ngx_string("qoi"),
    NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
    ngx_http_qoi,
    0,
    0,
    NULL
  },
  ngx_null_command
};

static ngx_http_module_t ngx_http_qoi_module_ctx = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

ngx_module_t ngx_http_qoi_module = {
  NGX_MODULE_V1,
  &ngx_http_qoi_module_ctx,
  ngx_http_qoi_commands,
  NGX_HTTP_MODULE,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NGX_MODULE_V1_PADDING
};
