// online/onlinebin-util.h

// Copyright 2012 Cisco Systems (author: Matthias Paulik)

//   Modifications to the original contribution by Cisco Systems made by:
//   Vassil Panayotov

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#ifndef KALDI_ONLINE_ONLINEBIN_UTIL_H_
#define KALDI_ONLINE_ONLINEBIN_UTIL_H_

#include "base/kaldi-common.h"
#include "fstext/fstext-lib.h"

// This file hosts the declarations of various auxiliary functions, used by
// the binaries in "onlinebin" directory. These functions are not part of the
// core online decoding infrastructure, but rather artefacts of the particular
// implementation of the binaries.该文件声明了各种各样的辅助函数，由onlinebin目录下的二进制文件使用。
//这些函数并非在线解码基础架构核心的一部分，而是执行二进制文件的特殊人为方式。

namespace kaldi {

//从文件中读取解码图
fst::Fst<fst::StdArc> *ReadDecodeGraph(std::string filename);

// Prints a string corresponding to (a possibly partial) decode result as
// and adds a "new line" character if "line_break" argument is true
//打印一个对应于解码结果(可能只是一部分)的字符串并且如果line_break值为真，
//添加一个"new line"字符
void PrintPartialResult(const std::vector<int32>& words,
                        const fst::SymbolTable *word_syms,
                        bool line_break);

} // namespace kaldi

#endif // KALDI_ONLINE_ONLINEBIN_UTIL_H_
