#ifndef DEFAULTS_H
#define DEFAULTS_H

const int defaultMonPort = 1492;
const char defaultMonAddr[] = "localhost";
const int defaultClientTimeoutSeconds = 60;
const double defaultMonTimeoutSeconds = 10;

const int defaultSysLockTimeoutSeconds = 10;
const char defaultLockFile[] = "/tmp/dpfs.lock";

const char defaultConfDir[] = "dpfs";
// buildConfPaths also prepends $HOME/.config and .
static const char * defaultConfPaths[] = {"/etc","/usr/local/etc"};
const char defaultConfFile[] = "dpfs.conf";

#endif
