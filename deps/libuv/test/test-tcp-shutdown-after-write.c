/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "task.h"

static void write_cb(uv_write_t* req, int status);
static void shutdown_cb(uv_shutdown_t* req, int status);

static uv_tcp_t conn;
static uv_timer_t timer;
static uv_connect_t connect_req;
static uv_write_t write_req;
static uv_shutdown_t shutdown_req;

static int connect_cb_called;
static int write_cb_called;
static int shutdown_cb_called;

static int conn_close_cb_called;
static int timer_close_cb_called;


static void close_cb(uv_handle_t* handle) {
  if (handle == (uv_handle_t*)&conn)
    conn_close_cb_called++;
  else if (handle == (uv_handle_t*)&timer)
    timer_close_cb_called++;
  else
    ASSERT(0 && "bad handle in close_cb");
}


static void alloc_cb(uv_handle_t* handle,
                     size_t suggested_size,
                     uv_buf_t* buf) {
  static char slab[64];
  buf->base = slab;
  buf->len = sizeof(slab);
}


static void timer_cb(uv_timer_t* handle) {
  uv_buf_t buf;
  int r;

  uv_close((uv_handle_t*)handle, close_cb);

  buf = uv_buf_init("TEST", 4);
  r = uv_write(&write_req, (uv_stream_t*)&conn, &buf, 1, write_cb);
  ASSERT(r == 0);

  r = uv_shutdown(&shutdown_req, (uv_stream_t*)&conn, shutdown_cb);
  ASSERT(r == 0);
}


static void read_cb(uv_stream_t* handle, ssize_t n_read, const uv_buf_t* buf) {
}


static void connect_cb(uv_connect_t* req, int status) {
  int r;

  ASSERT(status == 0);
  connect_cb_called++;

  r = uv_read_start((uv_stream_t*)&conn, alloc_cb, read_cb);
  ASSERT(r == 0);
}


static void write_cb(uv_write_t* req, int status) {
  ASSERT(status == 0);
  write_cb_called++;
}


static void shutdown_cb(uv_shutdown_t* req, int status) {
  ASSERT(status == 0);
  shutdown_cb_called++;
  uv_close((uv_handle_t*)&conn, close_cb);
}


TEST_IMPL(tcp_shutdown_after_write) {
  struct sockaddr_in addr;
  uv_loop_t* loop;
  int r;

  ASSERT(0 == uv_ip4_addr("127.0.0.1", TEST_PORT, &addr));
  loop = uv_default_loop();

  r = uv_timer_init(loop, &timer);
  ASSERT(r == 0);

  r = uv_timer_start(&timer, timer_cb, 125, 0);
  ASSERT(r == 0);

  r = uv_tcp_init(loop, &conn);
  ASSERT(r == 0);

  r = uv_tcp_connect(&connect_req,
                     &conn,
                     (const struct sockaddr*) &addr,
                     connect_cb);
  ASSERT(r == 0);

  r = uv_run(loop, UV_RUN_DEFAULT);
  ASSERT(r == 0);

  ASSERT(connect_cb_called == 1);
  ASSERT(write_cb_called == 1);
  ASSERT(shutdown_cb_called == 1);
  ASSERT(conn_close_cb_called == 1);
  ASSERT(timer_close_cb_called == 1);

  MAKE_VALGRIND_HAPPY(loop);
  return 0;
}
