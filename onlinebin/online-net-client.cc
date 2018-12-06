// onlinebin/online-net-client.cc

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
//netdb.h中包含了getaddrinfo函数
#include <netdb.h>
#include <fcntl.h>

#include "feat/feature-mfcc.h"
#include "online/online-audio-source.h"
#include "online/online-feat-input.h"


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;

    typedef kaldi::int32 int32;
    typedef OnlineFeInput<Mfcc> FeInput;

    //portaudio源的超时时间间隔
    const int32 kTimeout = 500; 
    // PortAudio的采样频率
    const int32 kSampleFreq = 16000;
    // PortAudio内部用户态缓冲字节数
    const int32 kPaRingSize = 32768;
    // Report interval for PortAudio buffer overflows in number of feat. batches
    const int32 kPaReportInt = 4;
    //用法：从麦克风接收输入，提取特征并且通过网络连接将它们发送到语音识别服务器
    const char *usage =
        "Takes input using a microphone(PortAudio), extracts features and sends them\n"
        "to a speech recognition server over a network connection\n\n"
        "Usage: online-net-client server-address server-port\n\n";
    ParseOptions po(usage);
    //一次性发送和提取的特征向量数量
    int32 batch_size = 27;
    po.Register("batch-size", &batch_size,
                "The number of feature vectors to be extracted and sent in one go");
    po.Read(argc, argv);
    //如果参数不为2 则输出函数用法并退出
    if (po.NumArgs() != 2) {
      po.PrintUsage();
      return 1;
    }
    //分别存放服务器地址字符串和服务器端口字符串
    std::string server_addr_str = po.GetArg(1);
    std::string server_port_str = po.GetArg(2);
    //addrinfo是一个结构体
    //hints中存放期望返回的信息类型
    addrinfo *server_addr, hints;
    //family指定协议簇
    hints.ai_family = AF_INET;  //指定协议簇为IPv4
    //protocol指定传输的协议
    hints.ai_protocol = IPPROTO_UDP;  //指定为UDP
    //指定套接字类型 
    hints.ai_socktype = SOCK_DGRAM;  //STREAM(字节流)为TCP DGRAM(数据报)为UDP 
    hints.ai_flags = AI_ADDRCONFIG;
    //把服务器ip地址和服务器端口号以及hints中的内容
    //转换成套接口地址结构赋值给server_addr对象(也是一个addrinfo结构体)
    //返回0表示成功，非0则出错
    if (getaddrinfo(server_addr_str.c_str(), server_port_str.c_str(),
                    &hints, &server_addr) != 0)
      KALDI_ERR << "getaddrinfo() call failed!";
    //套接字的描述符
    int32 sock_desc = socket(server_addr->ai_family,
                             server_addr->ai_socktype,
                             server_addr->ai_protocol);
    if (sock_desc == -1)
      KALDI_ERR << "socket() call failed!";
    int32 flags = fcntl(sock_desc, F_GETFL);
    flags |= O_NONBLOCK;
    if (fcntl(sock_desc, F_SETFL, flags) == -1)
      KALDI_ERR << "fcntl() failed to put the socket in non-blocking mode!";

    //设置mfcc特征的参数 包括是否采用能量 帧长 帧移等等
    MfccOptions mfcc_opts;
    mfcc_opts.use_energy = false;
    int32 frame_length = mfcc_opts.frame_opts.frame_length_ms = 25;
    int32 frame_shift = mfcc_opts.frame_opts.frame_shift_ms = 10;
    //au_src是一个在线音频源接口(onlineaudiosourceinterface)对象
    OnlinePaSource au_src(kTimeout, kSampleFreq, kPaRingSize, kPaReportInt);
    Mfcc mfcc(mfcc_opts);
    FeInput fe_input(&au_src, &mfcc,
                     frame_length * (kSampleFreq / 1000),
                     frame_shift * (kSampleFreq / 1000));
    std::cerr << std::endl << "Sending features to " << server_addr_str
              << ':' << server_port_str << " ... " << std::endl;
    char buf[65535];
    Matrix<BaseFloat> feats;
    while (1) {
      feats.Resize(batch_size, mfcc_opts.num_ceps, kUndefined);
      bool more_feats = fe_input.Compute(&feats);
      if (feats.NumRows() > 0) {
        std::stringstream ss;
        feats.Write(ss, true); // serialize features as binary data
        //无连接的数据报方式传输数据，
        //sendto函数返回实际发送的数据字节长度，发送错误时返回-1
        ssize_t sent = sendto(sock_desc,
                              ss.str().c_str(),
                              ss.str().size(), 0,
                              server_addr->ai_addr,
                              server_addr->ai_addrlen);
        //如果udp连接发送错误则报错，函数调用失败
        if (sent == -1)
          KALDI_ERR << "sendto() call failed!";
        //recvfrom函数返回接收到的字节长度 如果出错则返回-1
        ssize_t rcvd = recvfrom(sock_desc, buf, sizeof(buf), 0,
                                server_addr->ai_addr, &server_addr->ai_addrlen);
        //判断是否出错
        if (rcvd == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
          KALDI_ERR << "recvfrom() failed unexpectedly!";
        } else if (rcvd > 0) {
          buf[rcvd] = 0;
          std::cout << buf;
          std::cout.flush();
        }
      }
      if (!more_feats) break;
    }
    freeaddrinfo(server_addr);
    return 0;
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
} // main()
