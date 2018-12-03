//
// Created by ximin.chen@rokid.com on 2018-11-28.
//

#include "./child_process.h"

YODA_NS_BEGIN

namespace {
void allocUVPipeBuf(uv_handle_t *, size_t size, uv_buf_t *buf) {
  buf->base = (char *) malloc(size);
  buf->len = size;
}
}

ChildProcess::ChildProcess(const std::shared_ptr<JobConf> &conf) :
  IJobExecutor("ChildProcess"),
  _cp(),
  _conf(conf),
  _filePath{0},
  _pipe0(nullptr),
  _pipe1(nullptr),
  _pipe2(nullptr) {
  auto task = conf->task;
  YODA_SIXSIX_SASSERT(task, "conf task is empty");
  sprintf(_filePath, "/tmp/yoda-sixsix-%d-%d.sh", task->id, task->shellId);
}

ChildProcess::~ChildProcess() {
  YODA_SIXSIX_SASSERT(!_cp, "child process is not exit yet");
}

void ChildProcess::execute() {
  uv_fs_t openReq;
  int32_t r;
  auto loop = uv_default_loop();
  // fd
  r = uv_fs_open(loop, &openReq, _filePath, O_WRONLY | O_CREAT | S_IWUSR | S_IRUSR, 0777, nullptr);
  YODA_SIXSIX_FASSERT(r >= 0, "open %s error: %s", _filePath, uv_err_name(r));
  uv_fs_req_cleanup(&openReq);
  auto fd = (uv_file) openReq.result;

  auto shellSize = _conf->task->shell->size();
  auto bufSize = shellSize < 256 ? 256 : shellSize;
  if (bufSize < 256) {
    bufSize = 256;
  }
  auto buf = (char *) malloc(bufSize + 1);
  memcpy(buf, _conf->task->shell->c_str(), shellSize);
  buf[shellSize] = '\0';
  auto uv_buf = uv_buf_init(buf, (uint32_t) shellSize);
  uv_fs_t writeReq;
  r = uv_fs_write(loop, &writeReq, fd, &uv_buf, 1, -1, nullptr);
  YODA_SIXSIX_FASSERT(r >= 0, "write %s error: %s", _filePath, uv_err_name(r));
  uv_fs_req_cleanup(&writeReq);

  uv_fs_t closeReq;
  r = uv_fs_close(loop, &closeReq, fd, nullptr);
  YODA_SIXSIX_FASSERT(r >= 0, "close %s error: %s", _filePath, uv_err_name(r));
  uv_fs_req_cleanup(&closeReq);

  _pipe1 = YODA_SIX_SIX_MALLOC(uv_pipe_t);
  r = uv_pipe_init(loop, _pipe1, 0);
  YODA_SIXSIX_FASSERT(r >= 0, "pipe1 error: %s", uv_err_name(r));
  _pipe2 = YODA_SIX_SIX_MALLOC(uv_pipe_t);
  r = uv_pipe_init(loop, _pipe2, 0);
  YODA_SIXSIX_FASSERT(r >= 0, "pipe2 error: %s", uv_err_name(r));
  auto stream1 = (uv_stream_t *) _pipe1;
  auto stream2 = (uv_stream_t *) _pipe2;

  uv_stdio_container_t io[3];
  io[0].flags = UV_IGNORE;
  io[1].flags = (uv_stdio_flags) (UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  io[1].data.stream = stream1;
  io[2].flags = (uv_stdio_flags) (UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  io[2].data.stream = stream2;

  uv_process_options_t options;
  memset(&options, 0, sizeof(uv_process_options_t));
  const char *args[2];
  args[0] = _filePath;
  args[1] = nullptr;
  options.cwd = getcwd(buf, bufSize);
  options.file = args[0];
  options.args = const_cast<char **>(args);
  options.flags = UV_PROCESS_DETACHED;
  options.stdio = io;
  options.stdio_count = 3;
  _cp = (uv_process_t *) malloc(sizeof(uv_process_t));
  UV_MAKE_CB_WRAP3(_cp, cb, ChildProcess, onChildProcessExit,
                   uv_process_t, int64_t, int32_t);
  options.exit_cb = cb;

  r = uv_spawn(uv_default_loop(), _cp, &options);
  YODA_SIXSIX_FASSERT(r == 0, "spawn %s error: %s", _filePath, uv_err_name(r));
//  uv_unref((uv_handle_t*) _cp);
  UV_MAKE_CB_WRAP3(stream1, cb1, ChildProcess, onPipeData,
                   uv_stream_t, ssize_t, const uv_buf_t *);
  UV_MAKE_CB_WRAP3(stream2, cb2, ChildProcess, onPipeData,
                   uv_stream_t, ssize_t, const uv_buf_t *);
  r = uv_read_start(stream1, allocUVPipeBuf, cb1);
  YODA_SIXSIX_FASSERT(r == 0, "stdout read error %s", uv_err_name(r));
  r = uv_read_start(stream2, allocUVPipeBuf, cb2);
  YODA_SIXSIX_FASSERT(r == 0, "stderr read error %s", uv_err_name(r));

  free(buf);
}

int ChildProcess::stop() {
  if (_cp) {
    int32_t r = uv_process_kill(_cp, 15 /* SIGTERM */);
    if (r == UV_ESRCH) {
      r = 0;
    }
    if (r) {
      const char *err = uv_err_name(r);
      YODA_SIXSIX_FLOG_ERROR("stop child process %d error %s", _cp->pid, err);
    } else {
      YODA_SIXSIX_FLOG_INFO("stop child process %d succeed", _cp->pid);
    }
  }
  return 0;
}

void ChildProcess::onChildProcessExit(uv_process_t *,
                                      int64_t code,
                                      int32_t signal) {
  YODA_SIXSIX_FLOG_INFO("process exit: %" PRId64 " %d", code, signal);
  UV_MAKE_CB_WRAP1(_cp, cb, ChildProcess, onUVHandleClosed, uv_handle_t);
  uv_close((uv_handle_t *) _cp, cb);
}

void ChildProcess::onUVHandleClosed(uv_handle_t *) {
  YODA_SIXSIX_SLOG_INFO("child process closed");
  YODA_SIXSIX_SAFE_FREE(_cp);
  YODA_SIXSIX_SAFE_FREE(_pipe0);
  YODA_SIXSIX_SAFE_FREE(_pipe1);
  YODA_SIXSIX_SAFE_FREE(_pipe2);
  uv_fs_t unlinkReq;
  uv_fs_unlink(uv_default_loop(), &unlinkReq, _filePath, nullptr);
  uv_fs_req_cleanup(&unlinkReq);
}

void ChildProcess::onPipeData(uv_stream_t *stm, ssize_t nread,
                              const uv_buf_t *buf) {
  if (nread > 0) {
    if (stm == (uv_stream_t *) _pipe1) {
      YODA_SIXSIX_FLOG(stdout, "child-info", "%s", buf->base);
    } else {
      YODA_SIXSIX_FLOG(stderr, "child-error", "%s", buf->base);
    }
  }
}

YODA_NS_END