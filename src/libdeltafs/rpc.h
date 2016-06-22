#pragma once

/*
 * Copyright (c) 2015-2016 Carnegie Mellon University.
 *
 * All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include <string>
#include <vector>

#include "pdlfs-common/env.h"
#include "pdlfs-common/status.h"

namespace pdlfs {
namespace rpc {
class If;
class IfWrapper;
}

enum RPCMode { kServerClient, kClientOnly };

struct RPCOptions {
  RPCMode mode;
  std::string uri;
  int num_io_threads_;
  ThreadPool* extra_workers;
  rpc::If* fs;
  Env* env;
};

class RPC {
 public:
  RPC() {}
  virtual ~RPC();

  static RPC* Open(const RPCOptions&);

  virtual rpc::If* NewClient(const std::string& addr) = 0;
  virtual Status Start() = 0;
  virtual Status Stop() = 0;

 private:
  // No copying allowed
  void operator=(const RPC&);
  RPC(const RPC&);
};

namespace rpc {
class If {
 public:
  struct Message {
    int err;
    Slice contents;
    char buf[2048];
    std::string extra_buf;
  };

  If() {}
  virtual ~If();

#define ADD_RPC(OP) virtual void OP(Message& in, Message& out) = 0

  ADD_RPC(NONOP);
  ADD_RPC(FSTAT);
  ADD_RPC(MKDIR);
  ADD_RPC(FCRET);
  ADD_RPC(CHMOD);
  ADD_RPC(CHOWN);
  ADD_RPC(UNLNK);
  ADD_RPC(RMDIR);
  ADD_RPC(RENME);
  ADD_RPC(LOKUP);
  ADD_RPC(LSDIR);

#undef ADD_RPC

 private:
  // No copying allowed
  void operator=(const If&);
  If(const If&);
};

class IfWrapper : public If {
 public:
  explicit IfWrapper(If* base = NULL) : base_(base) {}
  virtual ~IfWrapper();

#define DEF_RPC(OP)                            \
  virtual void OP(Message& in, Message& out) { \
    if (base_ != NULL) {                       \
      base_->OP(in, out);                      \
    } else {                                   \
      out.err = 0xff;                          \
    }                                          \
  }

  DEF_RPC(NONOP);
  DEF_RPC(FSTAT);
  DEF_RPC(MKDIR);
  DEF_RPC(FCRET);
  DEF_RPC(CHMOD);
  DEF_RPC(CHOWN);
  DEF_RPC(UNLNK);
  DEF_RPC(RMDIR);
  DEF_RPC(RENME);
  DEF_RPC(LOKUP);
  DEF_RPC(LSDIR);

#undef DEF_RPC

 private:
  If* base_;
  // No copying allowed
  void operator=(const IfWrapper&);
  IfWrapper(const IfWrapper&);
};

}  // namespace rpc
}  // namespace pdlfs