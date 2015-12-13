#ifndef DEFAULTS_H
#define DEFAULTS_H

#include <stdint.h>

const uint16_t defaultMonPort = 1492;
const uint16_t defaultOSDPort = 1493;
const char defaultMonAddr[] = "localhost";
const int defaultClientTimeoutSeconds = 60;
const double defaultMonTimeoutSeconds = 10;

const int defaultSysLockTimeoutSeconds = 10;
const char defaultLockFile[] = "/tmp/dpfs.lock";

// buildConfPaths prepends $HOME/.config and .
const char defaultConfDir[] = "dpfs";
const char defaultConfFile[] = "dpfs.conf";
const char defaultFSInitFile[] = "init";
const int defaultReadSize = 512*1024;

#endif
