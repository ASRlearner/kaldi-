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
//服务器端
#include "feat/feature-mfcc.h"
#include "online/online-feat-input.h"
#include "online/online-decodable.h"
#include "online/online-faster-decoder.h"
#include "online/onlinebin-util.h"

namespace kaldi {
//下面是服务器传输的部分识别结果函数
//参数为词id openfst符号表 line_break判断换行 服务器套接字 客户端地址
//
void SendPartialResult(const std::vector<int32>& words,
                       const fst::SymbolTable *word_syms,
                       const bool line_break,
                       const int32 serv_sock,
                       const sockaddr_in &client_addr) {
  KALDI_ASSERT(word_syms != NULL);
  //std命名空间中的输入输出流
  std::stringstream sstream;
  //注意：这里的words向量为服务器解码得到的词id序列
  //接下来的工作是对于识别的词序列从words.txt中寻找
  for (size_t i = 0; i < words.size(); i++) {
    //根据词的id从word_syms即openfst符号表中寻找id对应的词
    std::string word = word_syms->Find(words[i]);
    //如果这个词为空 则报错
    if (word == "")
      KALDI_ERR << "Word-id " << words[i] <<" not in symbol table.";
    //将word输出并输出一个空格
    sstream << word << ' ';
  }
  //判断是否换行
  if (line_break)
    //输出换行
    sstream << "\n\n";
  //str()返回string对象
  //服务器向客户端传输的数据
  //client_addr是客户端ip地址结构
  //reinterpret_cast是强制类型转换 在这里将sockaddr_in类型转换成sockaddr类型
  ssize_t sent = sendto(serv_sock, sstream.str().c_str(), sstream.str().size(),
                        0, reinterpret_cast<const sockaddr*>(&client_addr),
                        sizeof(client_addr));
  //如果sent为-1则报错 sendto用于发送数据
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
    //初始化parseoptions对象
    ParseOptions po(usage);
    //设置参数的默认值
    //basefloat是kaldi中定义的浮点类型
    BaseFloat acoustic_scale = 0.1;
    int32 cmn_window = 600,
      min_cmn_window = 100; // 只在语音的开始阶段添加一秒的延迟
    int32 right_context = 4, left_context = 4;
    //该类的定义位于feature-functions.h的48行
    //存储delta特征的参数选项
    kaldi::DeltaFeaturesOptions delta_opts;
    //给po对象配置delta特征的参数
    delta_opts.Register(&po);
    //给po配置在线快速解码的参数选项
    OnlineFasterDecoderOpts decoder_opts;
    //给po配置在线特征矩阵的参数选项
    OnlineFeatureMatrixOptions feature_reading_opts;
    //给po对象配置解码器参数
    decoder_opts.Register(&po, true);
    feature_reading_opts.Register(&po);
    //register有三个参数其中 参数1和3是字符串 参数2是任意类型的数据
    //登记输入的参数选项值
    po.Register("left-context", &left_context, "Number of frames of left context");
    po.Register("right-context", &right_context, "Number of frames of right context");
    //声学模型比例
    po.Register("acoustic-scale", &acoustic_scale,
                "Scaling factor for acoustic likelihoods");
    po.Register("cmn-window", &cmn_window,
        "Number of feat. vectors used in the running average CMN calculation");
    po.Register("min-cmn-window", &min_cmn_window,
                "Minumum CMN window used at start of decoding (adds "
                "latency only at start)");
    //这个函数必须在所有变量登记完后进行调用
    po.Read(argc, argv);
    //如果参数个数不为5并且不为6 则输出使用信息
    if (po.NumArgs() != 5 && po.NumArgs() != 6) {
      po.PrintUsage();
      return 1;
    }
    //读取所有参数并且赋值给相应的变量 online-feat-input
    std::string model_rxfilename = po.GetArg(1),   //最终得到的模型  final.mdl
        fst_rxfilename = po.GetArg(2),               //fst解码图读取路径HCLG.fst
        word_syms_filename = po.GetArg(3),       //openfst符号表文件读取路径 words.txt(存放词到id的映射 服务器传回识别结果时使用)
        silence_phones_str = po.GetArg(4),      //静音音素  '1:2:3:4:5' 用来判断断点使用
        lda_mat_rspecifier = po.GetOptArg(6);      //lad特征矩阵读取  final.mat
    //atoi()实现将字符串转化为整形数字
    //因为c语言中不存在string类型 故c_str()将string类型转化成c中字符串样式
    int32 udp_port = atoi(po.GetArg(5).c_str());    //udp端口号 获取参数5的字符串赋值给udp_port
     
     //如果lda特征矩阵参数不为空
    //输入lda矩阵
    Matrix<BaseFloat> lda_transform;
    if (lda_mat_rspecifier != "") {
      bool binary_in;
      //ki是kaldi中input类的实例
      //以二进制方式输入lda特征矩阵
      Input ki(lda_mat_rspecifier, &binary_in);
      //stream()如果文件打开失败则报错，否则返回流中的内容
      //read(&istream,bool)从流中读取
      lda_transform.Read(ki.Stream(), binary_in);
    }
     //读取静音音素，存放在可变长数组vector容器中
    std::vector<int32> silence_phones;
    //如果无法将字符按":"拆分后转化成数字，则报错
    if (!SplitStringToIntegers(silence_phones_str, ":", false, &silence_phones))
        KALDI_ERR << "Invalid silence-phones string " << silence_phones_str;
    //如果未输入静音音素
    if (silence_phones.empty())
        KALDI_ERR << "No silence phones given!";
    //读取模型文件
    TransitionModel trans_model;
    //对角协方差混合高斯矩阵
    //输入最终训练得到的模型
    //将最终得到的模型文件以流的形式输入到对象转移模型和对角协方差高斯混合矩阵中
    AmDiagGmm am_gmm;
    {
        bool binary;
        //input可以读取任意类型的文件 默认使用非二进制模式
        Input ki(model_rxfilename, &binary);
        trans_model.Read(ki.Stream(), binary);
        am_gmm.Read(ki.Stream(), binary);
    }
    //读取词表文件words.txt
    fst::SymbolTable *word_syms = NULL;
    if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                    << word_syms_filename;
    //读取解码图HCLG.fst 
    //返回得到fst中存在弧类型的解码图
    fst::Fst<fst::StdArc> *decode_fst = ReadDecodeGraph(fst_rxfilename);

    // We are not properly registering/exposing MFCC and frame extraction options,
    // because there are parts of the online decoding code, where some of these
    // options are hardwired(ToDo: we should fix this at some point)
    //我们并未适当地登记/显示MFCC和帧提取选项，因为有部分的在线解码代码中的选项是固定的
    //在某些时候我们需要自己修改它
    //为mfcc特征配置参数
    MfccOptions mfcc_opts;
    //默认不使用对数能量
    mfcc_opts.use_energy = false;
    //创建快速解码对象decoder输入为
    //由openfst得到的解码图 解码器参数 静音音素和转移模型(由最终训练得到的模型)
    OnlineFasterDecoder decoder(*decode_fst, decoder_opts,
                                silence_phones, trans_model);
    //该类还未找到可能是存放了词图弧的向量
    VectorFst<LatticeArc> out_fst;
    //存放的mfcc倒谱系数即特征维度
    int32 feature_dim = mfcc_opts.num_ceps; // 当前默认13维.
    //udp_input对象存放了udp端口的一些配置信息
    //调用了该函数特征维度和udp端口号
    OnlineUdpInput udp_input(udp_port, feature_dim);
    //udp_input只是一个onlineudpinput对象
    OnlineCmnInput cmn_input(&udp_input, cmn_window, min_cmn_window);
    //定义在线特征输入接口对象(即之后真正传输的特征)
    OnlineFeatInputItf *feat_transform = 0;
    //判断是否添加了lda特征矩阵 如果是则使用lda作为输入
    //否则delta作为输入
    if (lda_mat_rspecifier != "") {
      //获取线性变换矩阵
      feat_transform = new OnlineLdaInput(
                               &cmn_input, lda_transform,
                               left_context, right_context);
    } else {
      DeltaFeaturesOptions opts;
      //这里默认设为2
      opts.order = kDeltaOrder;
      feat_transform = new OnlineDeltaInput(opts, &cmn_input);
    }

    // feature_reading_opts 包含了默认的每次27个特征传输个数以及5次超时放弃前的请求次数
    //在线特征矩阵对象保存了lda输入矩阵或者delta输入矩阵作为传输的特征
    OnlineFeatureMatrix feature_matrix(feature_reading_opts,
                                       feat_transform);
    //参数是对角协方差混合高斯矩阵 转移模型文件 声学模型比例 以及在线特征矩阵
    OnlineDecodableDiagGmmScaled decodable(am_gmm, trans_model, acoustic_scale,
                                           &feature_matrix);

    std::cerr << std::endl << "Listening on UDP port "
              << udp_port << " ... " << std::endl;
    bool partial_res = false;
    while (1) {
      //这里获得解码的状态 共三种 表示三种情况
      OnlineFasterDecoder::DecodeState dstate = decoder.Decode(&decodable);
      //用于存放识别出的词的id 打印部分识别结果时使用
      std::vector<int32> word_ids;
      if (dstate & (decoder.kEndFeats | decoder.kEndUtt)) {
        //将生成的词图放入out_fst中
        decoder.FinishTraceBack(&out_fst);
        //位于fstext/fstext-utils.h中
        fst::GetLinearSymbolSequence(out_fst,
                                     static_cast<vector<int32> *>(0),
                                     &word_ids,
                                     static_cast<LatticeArc::Weight*>(0));
        //传输部分结果
        SendPartialResult(word_ids, word_syms, partial_res || word_ids.size(),
                          udp_input.descriptor(), udp_input.client_addr());
        partial_res = false;
      } else {
        if (decoder.PartialTraceback(&out_fst)) {
          fst::GetLinearSymbolSequence(out_fst,
                                       static_cast<vector<int32> *>(0),
                                       &word_ids,
                                       static_cast<LatticeArc::Weight*>(0));
          //传输部分的结果
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
