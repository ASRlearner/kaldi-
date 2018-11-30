// onlinebin/online-server-gmm-decode-faster.cc

// Copyright 2012 Cisco Systems (author: Matthias Paulik)
//           2012 Vassil Panayotov
//           2013 Johns Hopkins University (author: Daniel Povey)

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

#include "feat/feature-mfcc.h"
#include "online/online-feat-input.h"
#include "online/online-decodable.h"
#include "online/online-faster-decoder.h"
#include "online/onlinebin-util.h"

namespace kaldi {

//参数为词id 词符号表 line_break还不清楚 服务器套接字 客户端地址
void SendPartialResult(const std::vector<int32>& words,
                       const fst::SymbolTable *word_syms,
                       const bool line_break,
                       const int32 serv_sock,
                       const sockaddr_in &client_addr) {
  KALDI_ASSERT(word_syms != NULL);
  std::stringstream sstream;
  for (size_t i = 0; i < words.size(); i++) {
    //根据词的id从word_syms即词符号表中寻找id对应的词
    std::string word = word_syms->Find(words[i]);
    //如果这个词为空 则报错
    if (word == "")
      KALDI_ERR << "Word-id " << words[i] <<" not in symbol table.";
    sstream << word << ' ';
  }
  if (line_break)
    sstream << "\n\n";

  ssize_t sent = sendto(serv_sock, sstream.str().c_str(), sstream.str().size(),
                        0, reinterpret_cast<const sockaddr*>(&client_addr),
                        sizeof(client_addr));
  //如果sent为-1则报错 sendto用于发送识别结果
  if (sent == -1)
    KALDI_WARN << "sendto() call failed when tried to send recognition results";
}

} // kaldi命名空间


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    using namespace fst;
    //这里确定使用kaldi和fst命名空间下的函数
    typedef kaldi::int32 int32;

    // Up to delta-delta derivative features are calculated (unless LDA is used)
    //在不使用LDA矩阵的情况下直接计算delta-delta导数特征
    const int32 kDeltaOrder = 2;

    const char *usage =
        "语音解码, 使用由网络连接接收的批度特征\n\n"
        "实时完成了语音的分段工作.\n"
        "如果给出最后一个参数，使用特征拼接/LDA变换\n\n"
        "Otherwise delta/delta-delta(2-nd order) features are produced.\n\n"
        "Usage: online-server-gmm-decode-faster [options] model-in"
        "fst-in word-symbol-table silence-phones udp-port [lda-matrix-in]\n\n"
        "Example: online-server-gmm-decode-faster --rt-min=0.3 --rt-max=0.5 "
        "--max-active=4000 --beam=12.0 --acoustic-scale=0.0769 "
        "model HCLG.fst words.txt '1:2:3:4:5' 1234 lda-matrix";
    //po是parseoptions对象，parseoptions用于读取命令行中的命令
    ParseOptions po(usage);
    //设置参数的默认值
    //basefloat是kaldi中定义的浮点类型
    BaseFloat acoustic_scale = 0.1;
    int32 cmn_window = 600,
      min_cmn_window = 100; // 只在语音的开始阶段添加一秒的延迟
    int32 right_context = 4, left_context = 4;
    //这一段还不清楚具体的作用 可能与解码相关
    kaldi::DeltaFeaturesOptions delta_opts;
    delta_opts.Register(&po);
    OnlineFasterDecoderOpts decoder_opts;
    OnlineFeatureMatrixOptions feature_reading_opts;
    decoder_opts.Register(&po, true);
    feature_reading_opts.Register(&po);
    //register有三个参数其中 参数1和3是字符串 参数2是任意类型的数据
    //登记输入的参数选项值
    po.Register("left-context", &left_context, "Number of frames of left context");
    po.Register("right-context", &right_context, "Number of frames of right context");
    po.Register("acoustic-scale", &acoustic_scale,
                "Scaling factor for acoustic likelihoods");
    po.Register("cmn-window", &cmn_window,
        "Number of feat. vectors used in the running average CMN calculation");
    po.Register("min-cmn-window", &min_cmn_window,
                "Minumum CMN window used at start of decoding (adds "
                "latency only at start)");

    po.Read(argc, argv);
    //如果参数个数不为5并且不为6 则输出使用信息
    if (po.NumArgs() != 5 && po.NumArgs() != 6) {
      po.PrintUsage();
      return 1;
    }
    //读取所有参数并且赋值给相应的变量 online-feat-input
    std::string model_rxfilename = po.GetArg(1),   //最终得到的模型  final.mdl
        fst_rxfilename = po.GetArg(2),               //fst解码图读取路径HCLG.fst
        word_syms_filename = po.GetArg(3),       //words文件读取路径 words.txt
        silence_phones_str = po.GetArg(4),      //静音音素  '1:2:3:4:5'
        lda_mat_rspecifier = po.GetOptArg(6);      //lad特征矩阵读取  final.mat
    //atoi()实现将字符串转化为整形数字
    //c_str()将string类型转化成c中字符串样式
    int32 udp_port = atoi(po.GetArg(5).c_str());    //udp端口号 获取参数5的字符串赋值给udp_port
     
     //如果lda特征矩阵参数不为空
    //输入lda矩阵
    Matrix<BaseFloat> lda_transform;
    if (lda_mat_rspecifier != "") {
      bool binary_in;
      //ki是kaldi中input类的实例
      //以二进制方式输入lda特征矩阵
      Input ki(lda_mat_rspecifier, &binary_in);
      lda_transform.Read(ki.Stream(), binary_in);
    }
     //读取静音音素
    std::vector<int32> silence_phones;
    //如果无法将字符按":"拆分后转化成数字，则报错
    if (!SplitStringToIntegers(silence_phones_str, ":", false, &silence_phones))
        KALDI_ERR << "Invalid silence-phones string " << silence_phones_str;
    //如果未输入静音音素
    if (silence_phones.empty())
        KALDI_ERR << "No silence phones given!";
    //以二进制方式读取模型文件
    TransitionModel trans_model;
    //对角协方差混合高斯矩阵
    //输入最终训练得到的模型
    AmDiagGmm am_gmm;
    {
        bool binary;
        Input ki(model_rxfilename, &binary);
        trans_model.Read(ki.Stream(), binary);
        am_gmm.Read(ki.Stream(), binary);
    }
    //读取词表文件
    fst::SymbolTable *word_syms = NULL;
    if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                    << word_syms_filename;

    fst::Fst<fst::StdArc> *decode_fst = ReadDecodeGraph(fst_rxfilename);

    // We are not properly registering/exposing MFCC and frame extraction options,
    // because there are parts of the online decoding code, where some of these
    // options are hardwired(ToDo: we should fix this at some point)
    //我们并未适当地登记/显示MFCC和帧提取选项，因为有部分的在线解码代码中的选项是固定的
    //在某些时候我们需要自己修改它
    MfccOptions mfcc_opts;
    mfcc_opts.use_energy = false;

    OnlineFasterDecoder decoder(*decode_fst, decoder_opts,
                                silence_phones, trans_model);
    VectorFst<LatticeArc> out_fst;
    int32 feature_dim = mfcc_opts.num_ceps; // default to 13 right now.
    //说明前面的代码都已经正确执行
    //调用了该函数特征维度和udp端口号
    OnlineUdpInput udp_input(udp_port, feature_dim);
    OnlineCmnInput cmn_input(&udp_input, cmn_window, min_cmn_window);
    OnlineFeatInputItf *feat_transform = 0;

    if (lda_mat_rspecifier != "") {
      feat_transform = new OnlineLdaInput(
                               &cmn_input, lda_transform,
                               left_context, right_context);
    } else {
      DeltaFeaturesOptions opts;
      opts.order = kDeltaOrder;
      feat_transform = new OnlineDeltaInput(opts, &cmn_input);
    }

    // feature_reading_opts contains number of retries, batch size.
    OnlineFeatureMatrix feature_matrix(feature_reading_opts,
                                       feat_transform);

    OnlineDecodableDiagGmmScaled decodable(am_gmm, trans_model, acoustic_scale,
                                           &feature_matrix);

    std::cerr << std::endl << "Listening on UDP port "
              << udp_port << " ... " << std::endl;
    bool partial_res = false;
    while (1) {
      OnlineFasterDecoder::DecodeState dstate = decoder.Decode(&decodable);
      std::vector<int32> word_ids;
      if (dstate & (decoder.kEndFeats | decoder.kEndUtt)) {
        decoder.FinishTraceBack(&out_fst);
        fst::GetLinearSymbolSequence(out_fst,
                                     static_cast<vector<int32> *>(0),
                                     &word_ids,
                                     static_cast<LatticeArc::Weight*>(0));
        SendPartialResult(word_ids, word_syms, partial_res || word_ids.size(),
                          udp_input.descriptor(), udp_input.client_addr());
        partial_res = false;
      } else {
        if (decoder.PartialTraceback(&out_fst)) {
          fst::GetLinearSymbolSequence(out_fst,
                                       static_cast<vector<int32> *>(0),
                                       &word_ids,
                                       static_cast<LatticeArc::Weight*>(0));
          SendPartialResult(word_ids, word_syms, false,
                            udp_input.descriptor(), udp_input.client_addr());
          if (!partial_res)
            partial_res = (word_ids.size() > 0);
        }
      }
    }

    delete feat_transform;
    delete word_syms;
    delete decode_fst;
    return 0;
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
} // main()
