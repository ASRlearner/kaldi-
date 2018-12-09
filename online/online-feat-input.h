// online/online-feat-input.h

// Copyright 2012 Cisco Systems (author: Matthias Paulik)
//           2012-2013  Vassil Panayotov
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

#ifndef KALDI_ONLINE_ONLINE_FEAT_INPUT_H_
#define KALDI_ONLINE_ONLINE_FEAT_INPUT_H_

#if !defined(_MSC_VER)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "online-audio-source.h"
#include "feat/feature-functions.h"

namespace kaldi {

//接口说明
class OnlineFeatInputItf {
 public:
  // 以一些方式生成特征向量
  //这些特征很可能是 例如从音频样本中接收并提取，或者从另一个在线特征输入类转换而来
  //
  // "output" - a matrix to store the extracted feature vectors in its rows.
  //            The number of rows (NumRows()) of "output" when the function is
  //            called, is treated as a hint of how many frames the user wants,
  //            but this function does not promise to produce exactly that many:
  //            it may be slightly more, less, or even zero, on a given call.
  //            Zero frames may be returned because we timed out or because
  //            we're at the beginning of the file and some buffering is going on.
  //            In that case you should try again.  The function will return "false"
  //            when it knows the stream is finished, but if it returns nothing
  //            several times in a row you may want to terminate processing the
  //            stream.按行存储了提取出的特征向量的矩阵。函数调用时输出的行数表明了用户需要的帧数
  //
  // Note: similar to the OnlineAudioInput::Read(), Compute() previously
  //       had a second argument - "timeout". Again we decided against including
  //       this parameter in the interface specification. Instead we are
  //       considering time out handling to be implementation detail, and if needed
  //       it should be configured, through the descendant class' constructor,
  //       or by other means.
  //       For consistency, we recommend 'timeout' values greater than zero
  //       to mean that Compute() should not block for more than that number
  //       of milliseconds, and to return whatever data it has, when the timeout
  //       period is exceeded.
  //
  // Returns "false" if we know the underlying data source has no more data, and
  // true if there may be more data.
  virtual bool Compute(Matrix<BaseFloat> *output) = 0;

  virtual int32 Dim() const = 0; // 返回这些特征的输出维度
  
  virtual ~OnlineFeatInputItf() {}
};


// Acts as a proxy to an underlying OnlineFeatInput.作为潜在在线特征输入的代理
// 运用倒谱均值归一化
class OnlineCmnInput: public OnlineFeatInputItf {
 public:
  // "input" - 未归一化的在线特征输入
  // "cmn_window" - the count of the preceding vectors over which the average is
  //                calculated
  // "min_window" - the minimum count of frames for which it will compute the
  //                mean, at the start of the file.  Adds latency but only at the
  //                start
  OnlineCmnInput(OnlineFeatInputItf *input, int32 cmn_window, int32 min_window)
      : input_(input), cmn_window_(cmn_window), min_window_(min_window),
        history_(cmn_window + 1, input->Dim()), t_in_(0), t_out_(0),
        sum_(input->Dim()) { KALDI_ASSERT(cmn_window >= min_window && min_window > 0); }
  
  virtual bool Compute(Matrix<BaseFloat> *output);

  virtual int32 Dim() const { return input_->Dim(); }

 private:
  virtual bool ComputeInternal(Matrix<BaseFloat> *output);

  
  OnlineFeatInputItf *input_;
  const int32 cmn_window_; // > 0
  const int32 min_window_; // > 0, < cmn_window_.
  Matrix<BaseFloat> history_; // circular-buffer history, of dim (cmn_window_ +
                              // 1, feat-dim).  The + 1 is to serve as a place
                              // for the frame we're about to normalize.

  void AcceptFrame(const VectorBase<BaseFloat> &input); // Accept the next frame
                                                        // of input (read into the
                                                        // history buffer).
  void OutputFrame(VectorBase<BaseFloat> *output); // Output the next frame.
  
  int32 NumOutputFrames(int32 num_new_frames,
                        bool more_data) const; // Tells the caller, assuming
  // we get given "num_new_frames" of input (and given knowledge of whether
  // there is more data coming), how many frames would we be able to
  // output?
  
  
  int64 t_in_; // Time-counter for what we've obtained from the input.
  int64 t_out_; // Time-counter for what we've written to the output.
  
  Vector<double> sum_; // Sum of the frames from t_out_ - HistoryLength(t_out_),
                       // to t_out_ - 1.
  
  KALDI_DISALLOW_COPY_AND_ASSIGN(OnlineCmnInput);
};

//继承自OnlineFeatInputItf类
class OnlineCacheInput : public OnlineFeatInputItf {
 public:
  OnlineCacheInput(OnlineFeatInputItf *input): input_(input) { }
  
  // The Compute function just forwards to the previous member of the
  // chain, except that we locally accumulate the result, and
  // GetCachedData() will return the entire input up to the current time.
  virtual bool Compute(Matrix<BaseFloat> *output);

  void GetCachedData(Matrix<BaseFloat> *output);
  
  int32 Dim() const { return input_->Dim(); }
  
  void Deallocate();
    
  virtual ~OnlineCacheInput() { Deallocate(); }
  
 private:
  OnlineFeatInputItf *input_;
  // data_ is a list of all the outputs we produced in successive
  // calls to Compute().  The memory is owned here.
  //data_是我们持续调用compute函数生成的所有输出的列表
  std::vector<Matrix<BaseFloat>* > data_;
};


#if !defined(_MSC_VER)

// Accepts features over an UDP socket
// The current implementation doesn't support the "timeout" -
// the server is waiting for data indefinetily long time.
//接收udp套接字传来的特征 目前的实现不支持暂停——服务器长时间等待数据的到来
class OnlineUdpInput : public OnlineFeatInputItf {
 public:
  OnlineUdpInput(int32 port, int32 feature_dim);

  virtual bool Compute(Matrix<BaseFloat> *output);
  //特征维度
  virtual int32 Dim() const { return feature_dim_; }
  //客户端地址
  const sockaddr_in& client_addr() const { return client_addr_; }
  //返回套接字描述符
  const int32 descriptor() const { return sock_desc_; }
  
 private:
  int32 feature_dim_;
  // various BSD sockets-related data structures
  int32 sock_desc_; // 套接字描述符
  sockaddr_in server_addr_;   //套接字服务器地址
  sockaddr_in client_addr_;   //套接字客户端地址
};

#endif


// Splices the input features and applies a transformation matrix.
// Note: the transformation matrix will usually be a linear transformation
// [output-dim x input-dim] but we accept an affine transformation too.
//拼接输入特征并且运用一个转换矩阵。
//注意：转换矩阵通常是线性变换
//只有在输入了lda矩阵的情况下调用这个类
class OnlineLdaInput: public OnlineFeatInputItf {
 public:
  OnlineLdaInput(OnlineFeatInputItf *input,
                 const Matrix<BaseFloat> &transform,
                 int32 left_context,
                 int32 right_context);

  virtual bool Compute(Matrix<BaseFloat> *output);

  virtual int32 Dim() const { return linear_transform_.NumRows(); }

 private:
  // The static function SpliceFeats splices together the features and
  // puts them together in a matrix, so that each row of "output" contains
  // a contiguous window of size "context_window" of input frames.  The dimension
  // of "output" will be feats.NumRows() - context_window + 1 by
  // feats.NumCols() * context_window.  The input features are
  // treated as if the frames of input1, input2 and input3 have been appended
  // together before applying the main operation.
  static void SpliceFrames(const MatrixBase<BaseFloat> &input1,
                           const MatrixBase<BaseFloat> &input2,
                           const MatrixBase<BaseFloat> &input3,
                           int32 context_window,
                           Matrix<BaseFloat> *output);

  void TransformToOutput(const MatrixBase<BaseFloat> &spliced_feats,
                         Matrix<BaseFloat> *output);
  void ComputeNextRemainder(const MatrixBase<BaseFloat> &input);
  
  OnlineFeatInputItf *input_; // underlying/inferior input object
  const int32 input_dim_; // dimension of the feature vectors before xform
  const int32 left_context_;
  const int32 right_context_;
  Matrix<BaseFloat> linear_transform_; // LDA转换矩阵 (只有线性部分)
  Vector<BaseFloat> offset_; // Offset, if present; else empty.
  Matrix<BaseFloat> remainder_; // The last few frames of the input, that may
  // be needed for context purposes.
  
  KALDI_DISALLOW_COPY_AND_ASSIGN(OnlineLdaInput);
};


// 计算时间导数 (e.g., 添加 deltas 和 delta-deltas).
// 这是更加陈旧的特征提取标准.  Like an online
// version of the function ComputeDeltas in feat/feature-functions.h, where the
// class DeltaFeaturesOptions is also defined.
class OnlineDeltaInput: public OnlineFeatInputItf {
 public:
  OnlineDeltaInput(const DeltaFeaturesOptions &delta_opts,
                   OnlineFeatInputItf *input);
  
  virtual bool Compute(Matrix<BaseFloat> *output);

  virtual int32 Dim() const { return input_dim_ * (opts_.order + 1); }
  
 private:
  // The static function AppendFrames appends together the three input matrices,
  // some of which may be empty.
  static void AppendFrames(const MatrixBase<BaseFloat> &input1,
                           const MatrixBase<BaseFloat> &input2,
                           const MatrixBase<BaseFloat> &input3,
                           Matrix<BaseFloat> *output);

  // Context() is the number of frames on each side of a given frame,
  // that we need for context.
  int32 Context() const { return opts_.order * opts_.window; }
  
  // Does the delta computation.  Here, "output" will be resized to dimension
  // (input.NumRows() - Context() * 2) by (input.NumCols() * opts_.order)
  // "remainder" will be the last Context() rows of "input".
  void DeltaComputation(const MatrixBase<BaseFloat> &input,
                        Matrix<BaseFloat> *output,
                        Matrix<BaseFloat> *remainder) const;
  
  OnlineFeatInputItf *input_; // underlying/inferior input object
  DeltaFeaturesOptions opts_;
  const int32 input_dim_;
  Matrix<BaseFloat> remainder_; // The last few frames of the input, that may
  // be needed for context purposes.
  
  KALDI_DISALLOW_COPY_AND_ASSIGN(OnlineDeltaInput);
};

// Implementation, that is meant to be used to read samples from an
// OnlineAudioSource and to extract MFCC/PLP features in the usual way
//执行，意味着以通常的方式从一个onlineaudiosource对象中读取样本并且提取mfcc/plp特征
template <class E>
class OnlineFeInput : public OnlineFeatInputItf {
 public:
  // "au_src" - OnlineAudioSourceItf对象
  // "fe" - 执行MFCC/PLP特征提取的对象
  // "frame_size" - 音频样本的帧提取窗口大小
  // "frame_shift" - 音频样本的特征帧宽度
  OnlineFeInput(OnlineAudioSourceItf *au_src, E *fe,
                const int32 frame_size, const int32 frame_shift);

  virtual int32 Dim() const { return extractor_->Dim(); }

  virtual bool Compute(Matrix<BaseFloat> *output);

 private:
  OnlineAudioSourceItf *source_; // 音频资源
  E *extractor_; // 实际使用的特征提取器
  const int32 frame_size_;
  const int32 frame_shift_;
  Vector<BaseFloat> wave_; // 传输后用于提取的样本

  KALDI_DISALLOW_COPY_AND_ASSIGN(OnlineFeInput);
};

template<class E>
OnlineFeInput<E>::OnlineFeInput(OnlineAudioSourceItf *au_src, E *fe,
                                   int32 frame_size, int32 frame_shift)
    : source_(au_src), extractor_(fe),
      frame_size_(frame_size), frame_shift_(frame_shift) {}

template<class E> bool
OnlineFeInput<E>::Compute(Matrix<BaseFloat> *output) {
  MatrixIndexT nvec = output->NumRows(); // 输出特征的数量
  if (nvec <= 0) {
    KALDI_WARN << "No feature vectors requested?!";
    return true;
  }

  
  //准备输入音频样本
  int32 samples_req = frame_size_ + (nvec - 1) * frame_shift_;
  Vector<BaseFloat> read_samples(samples_req);

  bool ans = source_->Read(&read_samples);

  // 提取特征
  if (read_samples.Dim() >= frame_size_) {
    extractor_->Compute(read_samples, 1.0, output);
  } else {
    output->Resize(0, 0);
  }

  return ans;
}

struct OnlineFeatureMatrixOptions {
  int32 batch_size; // 每一次请求的帧数
  int32 num_tries; // 在放弃请求之前得到空输出和超时的尝试次数

  OnlineFeatureMatrixOptions(): batch_size(27),
                                num_tries(5) { }
  void Register(OptionsItf *opts) {
    opts->Register("batch-size", &batch_size,
                   "Number of feature vectors processed w/o interruption");
    opts->Register("num-tries", &num_tries,
                   "Number of successive repetitions of timeout before we "
                   "terminate stream");
  }
};

// The class OnlineFeatureMatrix wraps something of type
// OnlineFeatInputItf in a manner that is convenient for
// a Decodable type to consume.
class OnlineFeatureMatrix {
 public:
  OnlineFeatureMatrix(const OnlineFeatureMatrixOptions &opts,
                      OnlineFeatInputItf *input):
      opts_(opts), input_(input), feat_dim_(input->Dim()),
      feat_offset_(0), finished_(false) { }
  
  bool IsValidFrame (int32 frame); 

  int32 Dim() const { return feat_dim_; }

  //如果不是有效的帧  GetFrame() 调用失败; you have to
  // call IsValidFrame() for this frame, to see whether it
  // is valid.
  SubVector<BaseFloat> GetFrame(int32 frame);

  bool Good(); // 如果我们至少拥有一帧返回真值
 private:
  void GetNextFeatures(); //当我们需要更多特征时调用.确保获得至少一帧或者将finished_设为true
  
  const OnlineFeatureMatrixOptions opts_;
  OnlineFeatInputItf *input_;
  int32 feat_dim_;
  Matrix<BaseFloat> feat_matrix_;
  int32 feat_offset_; // 目前批次下第一帧的帧移
  bool finished_; // 如果没有得到更多的输入帧则为真
};


} //  kaldi命名空间

#endif // KALDI_ONLINE_ONLINE_FEAT_INPUT_H_
