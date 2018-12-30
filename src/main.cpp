//
// Created by ximin.chen@rokid.com on 2018/11/19.
//
#include "job_manager.h"
#include "options.h"
#include "WebSocketClient.h"
#include "MessageCommon.h"
#include "device_info.h"

using namespace rokid;

static const char *version = "v1.0.0\n";
static const char *helpStr =
  "Usage: \n"
  "[-version]         print version\n"
  "[-sysroot]         set sysroot\n"
  "[-taskJson]        start task from local json path\n"
  "[-disableUpload]   set none-zero to disable upload data\n"
  "[-serverAddress]   set server address\n"
  "[-serverPort]      set server port\n"
  "[-sn]              mock an sn\n"
  "[-hardware]        mock a hardware\n"
  "[-unzipRoot]       set task files unzip path\n"
  "[-smapInterval]    set smap collect interval in millisecond"
                      ", default is 3001000ms\n"
  "[-smapSleep]       set usleep time after collected a process in millisecond"
                      ", default is 1000ms\n"
  "\n\n"
  "Hints: it is appropriate to set smapSleep to 200 for Rokid Glass\n"
  "                            set smapSleep to 500 for A113\n"
  "                            set smapSleep to 1000 for Rokid Kamino\n";

static void parseExitCmd(int argc, char **argv) {
  if (argc >= 2) {
    const char *print = nullptr;
    if (strcmp(argv[1], "-help") == 0) {
      print = helpStr;
    } else if (strcmp(argv[1], "-version") == 0) {
      print = version;
    }
    if (print) {
      printf("%s", print);
      exit(0);
    }
  }
}

int main(int argc, char **argv) {
  setpriority(PRIO_PGRP, 0, 19);
  parseExitCmd(argc, argv);
  YODA_SIXSIX_SLOG("starting app");
  yoda::DeviceInfo::init();
  yoda::Options::parseCmdLine(argc, argv);

  yoda::JobManager manager;
  manager.startTaskFromCmdConf();

  WebSocketClient wsc(uv_default_loop(), 5);

  char path[128];
  auto serverAddress = yoda::Options::get<std::string>("serverAddress", "");
  auto serverPort = yoda::Options::get<uint32_t>("serverPort", 0);
  auto mockSN = yoda::Options::get<std::string>("sn", "");
  auto mockHardware = yoda::Options::get<std::string>("hardware", "");
  std::string sn = mockSN.empty() ? yoda::DeviceInfo::sn : mockSN;
  std::string hardware = mockHardware.empty() ?
    yoda::DeviceInfo::hardware : mockHardware;
  sprintf(path, "/websocket/%s/%s", sn.c_str(), hardware.c_str());
  if (serverAddress.empty() || serverPort == 0) {
    YODA_SIXSIX_FERROR("ws connect error, server: %s, port: %d",
                       serverAddress.c_str(), serverPort);
  } else {
    YODA_SIXSIX_FLOG("ws connect to %s:%d%s",
                     serverAddress.c_str(), serverPort, path);
    manager.setWs(&wsc);
    wsc.start(serverAddress.c_str(), serverPort, path);
  }

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  int32_t r = uv_loop_close(uv_default_loop());
  YODA_SIXSIX_FASSERT(r == 0, "uv loop close error %s", uv_err_name(r));
  wsc.stop();
  YODA_SIXSIX_SLOG("app finished");
  return 0;
}
