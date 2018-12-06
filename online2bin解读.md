# online2-wav-gmm-latgen-faster.cc

### 用途

读取wav文件并且模拟在线解码，包括基本fmllr适应和截断，写入词图。模型由选项来具体化。

### 使用方法

```
online2-wav-gmm-latgen-faster  [options]  <fst-in> <spk2utt-rspecifier> <wav-rspecifier> <lattice-wspecifier>
```

####  示例

```shell
    online2-wav-gmm-latgen-faster --do-endpointing=$do_endpointing \
     --config=$srcdir/conf/online_decoding.conf \
     --max-active=$max_active --beam=$beam --lattice-beam=$lattice_beam \
     --acoustic-scale=$acwt --word-symbol-table=$graphdir/words.txt \
     $graphdir/HCLG.fst $spk2utt_rspecifier "$wav_rspecifier" \
      "ark:|gzip -c > $dir/lat.JOB.gz" || exit 1;
```

### 参数意义

fst-in :  完全解码图fst文件(模型最终的HCLG.fst文件)

spk2utt-rspecifier : spk2utt的scp文件(存放说话人到语音映射的scp文件)

wav-rspecifier :  音频的scp文件(wav.scp,存放wav的路径地址)

lattice-wspecifier : 输出的lattice(压缩文件,如lat.JOB.gz)

### 选项列表

```
  --acoustic-scale            : 声学似然度的取值因素 (float, default = 0.1)
  --adaptation-delay          : Delay before first basis-fMLLR adaptation for not-first utterances of each speaker (float, default = 5)
  --adaptation-first-utt-delay : Delay before first basis-fMLLR adaptation for first utterance of each speaker (float, default = 2)
  --adaptation-first-utt-ratio : Ratio that controls frequency of fMLLR adaptation for first utterance of each speaker (float, default = 1.5)
  --adaptation-ratio          : Ratio that controls frequency of fMLLR adaptation for not-first utterances of each speaker (float, default = 2)
  --add-deltas                : 添加delta特征. (bool, default = false)
  --add-pitch                 : Append pitch features to raw MFCC/PLP features. (bool, default = false)
  --basis.fmllr-min-count     : Minimum count required to update fMLLR (float, default = 50)
  --basis.num-iters           : Number of iterations in basis fMLLR update during testing (int, default = 10)
  --basis.size-scale          : Scale (< 1.0) on speaker occupancy that gives number of basis elements. (float, default = 0.2)
  --basis.step-size-iters     : Number of iterations in computing step size (int, default = 3)
  --beam                      : Decoding beam.  Larger->slower, more accurate. (float, default = 16)
  --beam-delta                : Increment used in decoding-- this parameter is obscure and relates to a speedup in the way the max-active constraint is applied.  Larger is more accurate. (float, default = 0.5)
  --chunk-length              : Length of chunk size in seconds, that we process. (float, default = 0.05)
  --cmvn-config               : 在线cmvn特征的配置类文件 (e.g. conf/online_cmvn.conf) (string, default = "")
  --delta                     : Tolerance used in determinization (float, default = 0.000976562)
  --delta-config              :用于delta特征计算的配置文件(如果不使用, 则不使用delta特征; supply empty config to use defaults.) (string, default = "")
  --determinize-lattice       :如果该值为真，确定化词图(词图确定化,只保留每个词序列的最佳概率密度序列). (bool, default = true)
  --do-endpointing            : 如果该值为真，运用断点探测(bool, default = false)
  --endpoint.silence-phones   : List of phones that are considered to be silence phones by the endpointing code. (string, default = "")
  --fbank-config              : Configuration file for filterbank features (e.g. conf/fbank.conf) (string, default = "")
  --feature-type              : Base feature type [mfcc, plp, fbank] (string, default = "mfcc")
  --fmllr-basis               : (Extended) filename of fMLLR basis object, as output by gmm-basis-fmllr-training (string, default = "")
  --fmllr-lattice-beam        : Beam used in pruning lattices for fMLLR estimation (float, default = 3)
  --global-cmvn-stats         : (Extended) filename for global CMVN stats, e.g. obtained from 'matrix-sum scp:data/train/cmvn.scp -' (string, default = "")
  --hash-ratio                : Setting used in decoder to control hash behavior (float, default = 2)
  --lattice-beam              : Lattice generation beam.  Larger->slower, and deeper lattices (float, default = 10)
  --lda-matrix                : Filename of LDA matrix (if using LDA), e.g. exp/foo/final.mat (string, default = "")
  --max-active                : Decoder max active states.  Larger->slower; more accurate (int, default = 2147483647)
  --max-mem                   : Maximum approximate memory usage in determinization (real usage might be many times this). (int, default = 50000000)
  --mfcc-config               : Configuration file for MFCC features (e.g. conf/mfcc.conf) (string, default = "")
  --min-active                : Decoder minimum #active states. (int, default = 200)
  --minimize                  : If true, push and minimize after determinization. (bool, default = false)
  --model                     : (Extended) filename for model, typically the one used for fMLLR computation.  Required option. (string, default = "")
  --online-alignment-model    : (Extended) filename for model trained with online CMN features, e.g. from apply-cmvn-online. (string, default = "")
  --phone-determinize         : If true, do an initial pass of determinization on both phones and words (see also --word-determinize) (bool, default = true)
  --pitch-config              : Configuration file for pitch features (e.g. conf/pitch.conf) (string, default = "")
  --pitch-process-config      : Configuration file for post-processing pitch features (e.g. conf/pitch_process.conf) (string, default = "")
  --plp-config                : Configuration file for PLP features (e.g. conf/plp.conf) (string, default = "")
  --prune-interval            : Interval (in frames) at which to prune tokens (int, default = 25)
  --rescore-model             : (Extended) 用于给词图重新打分的模型文件名, e.g. discriminatively trainedmodel, if it differs from that supplied to --model option. 必须有相同的树. (string, default = "")
  --silence-phones            : 用冒号分隔的静音音素整数id, e.g. 1:2:3 (影响自适应). (string, default = "")
  --silence-weight            : 运用到fMLLR估计的静音帧的权重 (如果提供--silence-phones选项) (float, default = 0.1)
  --splice-config             : 付过有的话用做帧拼接的配置文件 (e.g. prior to LDA) (string, default = "")
  --splice-feats              : 使用左右上下文拼接特征. (bool, default = false)
  --word-determinize          : 如果该值为真, do a second pass of determinization on words only (see also --phone-determinize) (bool, default = true)
  --word-symbol-table         : 词符号表 [for debug output] (string, default = "")

```



### 注意事项



# ivector-randomize

### 用途

复制在线估计的ivector特征矩阵，同时随机化处理这些矩阵；这主要是用来使用ivector特征来训练在线nnet2而设定的；对于每个输入矩阵，索引为t的每一行带有所给选项的概率 --随机化概率,使用选中的输入行随机化代替从区间t到T的内容，T是矩阵最后一行的索引值

### 使用方法

```
ivector-randomize [options] <ivector-rspecifier> <ivector-wspecifier> 
```

#### 示例

```
ivector-randomize ark:- ark:-
```

### 参数意义

ivector-rspecifier : 特征矩阵的输入

ivector-wspecifier：特征矩阵的输出(可以是标准输出)

### 选项列表

srand: 随机数生成种子

randomize-prob: 对于每一行，使用带有该概率的随机行代替

### 注意事项





# online2-wav-dump-features

### 用途

读取wav文件并且像online2-wav-nnet2-lagten-faster一样处理它们，但是并非解码，而是转移存储特征。绝大多数参数经由配置变量设定。如果你想要逐条语音地生成特征，spk2utt读取的可以是<语音id><语音id>。

### 使用方法

```
online2-wav-dump-features [options] <spk2utt-rspecifier> <wav-rspecifier> <feature-wspecifier>

online2-wav-dump-features [options] --print-ivector-dim=true
```

#### 示例

```
online2-wav-dump-features  
--config=$dir/conf/online_feature_pipeline.conf \
      ark:$dir/spk2utt_fake/spk2utt.JOB "$wav_rspecifier" ark:-
```

### 参数意义

spk2utt-rspecifier: 说话人到语音的读取

wav-rspecifier: 音频文件的读取

feature-wspecifier: 特征的输出

### 选项列表

```
  --add-pitch                 : 给未加工的MFCC/PLP/filterbank特征添加音高特征 [iVector特征提取中不适用] (bool, 默认为false)
  --chunk-length              : 我们每秒处理的块长度大小. (float, 默认为0.05)
  --fbank-config              : 滤波器组特征的配置文件(e.g. conf/fbank.conf) (string, default = "")
  --feature-type              : 基础特征类型 [mfcc, plp, fbank] (string, default = "mfcc")
  --ivector-extraction-config : 在线iVector特征提取的配置文件, 看代码中的OnlineIvectorExtractionConfig类 (string, default = "")
  --ivector-silence-weighting.max-state-duration : (RE weighting in iVector estimation for online decoding) 单一transition-id所允许的最大持续时间; runs with durations longer than this will be weighted down to the silence-weight. (float, default = -1)
  --ivector-silence-weighting.silence-phones : (RE weighting in iVector estimation for online decoding) List of integer ids of silence phones, separated by colons (or commas).  Data that (according to the traceback of the decoder) corresponds to these phones will be downweighted by --silence-weight. (string, default = "")
  --ivector-silence-weighting.silence-weight : (RE weighting in iVector estimation for online decoding) Weighting factor for frames that the decoder trace-back identifies as silence; only relevant if the --silence-phones option is set. (float, default = 1)
  --mfcc-config               : MFCC特征的配置文件 (e.g. conf/mfcc.conf) (string, default = "")
  --online-pitch-config       : 在线音高特征的配置文件, 如果选项 --add-pitch=true (例如conf/online_pitch.conf) (string, default = "")
  --plp-config                : plp特征的配置文件(例如 conf/plp.conf) (string, default = "")
  --print-ivector-dim         : 如果为真值，打印ivector特征维度(可能为0)并退出.  This version requires no arguments. (bool, 默认为false)

```



### 注意事项



# online2-wav-nnet2-am-compute

### 用途

模拟在线神经网络对于每一个文件输入特征的计算并且输出一个带有任意的基于ivector的说话人自适应结果矩阵。注意：一些配置值和输入经由选项传输的配置文件设定。主要用于调试。另外，如果想要加入对数(例如对数似然度)，使用apply-log=true

### 使用方法

```
online2-wav-nnet2-am-compute [options] <nnet-in>
<spk2utt-rspecifier> <wav-rspecifier> <feature-or-loglikes-wspecifier>
```

#### 示例

### 参数意义

```
--add-pitch                 : Append pitch features to raw MFCC/PLP/filterbank features [but not for iVector extraction] (bool, default = false)
  --apply-log                 : Apply a log to the result of the computation before outputting. (bool, default = false)
  --chunk-length              : Length of chunk size in seconds, that we process. (float, default = 0.05)
  --fbank-config              : Configuration file for filterbank features (e.g. conf/fbank.conf) (string, default = "")
  --feature-type              : Base feature type [mfcc, plp, fbank] (string, default = "mfcc")
  --ivector-extraction-config : Configuration file for online iVector extraction, see class OnlineIvectorExtractionConfig in the code (string, default = "")
  --ivector-silence-weighting.max-state-duration : (RE weighting in iVector estimation for online decoding) Maximum allowed duration of a single transition-id; runs with durations longer than this will be weighted down to the silence-weight. (float, default = -1)
  --ivector-silence-weighting.silence-phones : (RE weighting in iVector estimation for online decoding) List of integer ids of silence phones, separated by colons (or commas).  Data that (according to the traceback of the decoder) corresponds to these phones will be downweighted by --silence-weight. (string, default = "")
  --ivector-silence-weighting.silence-weight : (RE weighting in iVector estimation for online decoding) Weighting factor for frames that the decoder trace-back identifies as silence; only relevant if the --silence-phones option is set. (float, default = 1)
  --mfcc-config               : Configuration file for MFCC features (e.g. conf/mfcc.conf) (string, default = "")
  --online                    : You can set this to false to disable online iVector estimation and have all the data for each utterance used, even at utterance start.  This is useful where you just want the best results and don't care about online operation.  Setting this to false has the same effect as setting --use-most-recent-ivector=true and --greedy-ivector-extractor=true in the file given to --ivector-extraction-config, and --chunk-length=-1. (bool, default = true)
  --online-pitch-config       : Configuration file for online pitch features, if --add-pitch=true (e.g. conf/online_pitch.conf) (string, default = "")
  --pad-input                 : If true, duplicate the first and last frames of input features as required for temporal context, to prevent #frames of output being less than those of input. (bool, default = true)
  --plp-config                : Configuration file for PLP features (e.g. conf/plp.conf) (string, default = "")

```

### 选项列表

nnet-in: 神经网络最终模型文件

spk2utt-rspecifier：说话人到语音的读取文件

wav-rspecifier: 音频的读取文件

feature-or-loglikes-wspecifier: 特征或者对数似然度的输出 

### 注意事项

