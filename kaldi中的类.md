# OptionsItf类(抽象类)

关于该类的描述位于itf/options-itf.h

公共成员函数

```c++
virtual void 	Register (const std::string &name, bool *ptr, const std::string &doc)=0
 
virtual void 	Register (const std::string &name, int32 *ptr, const std::string &doc)=0
 
virtual void 	Register (const std::string &name, uint32 *ptr, const std::string &doc)=0
 
virtual void 	Register (const std::string &name, float *ptr, const std::string &doc)=0
 
virtual void 	Register (const std::string &name, double *ptr, const std::string &doc)=0
 
virtual void 	Register (const std::string &name, std::string *ptr, const std::string &doc)=0
 
virtual 	~OptionsItf ()
```

提供了6个纯虚函数用于给不同类型(布尔型，整型，无符号整型，单精度浮点数，双精度浮点数，string)的参数赋值以及一个析构函数。

### ParseOptions类

用于解析命令行选项的类，继承自OptionsItf类

公有成员函数

```c++
	ParseOptions (const char *usage)
 
 	ParseOptions (const std::string &prefix, OptionsItf *other)
 	//这是以防在某些特殊情况下登记带有前缀的选项而产生冲突的构造函数
 
 	~ParseOptions ()
 
void Register (const std::string &name, bool *ptr, const std::string &doc)
 
void Register (const std::string &name, int32 *ptr, const std::string &doc)
 
void Register (const std::string &name, uint32 *ptr, const std::string &doc)
 
void Register (const std::string &name, float *ptr, const std::string &doc)
 
void Register (const std::string &name, double *ptr, const std::string &doc)
 
void Register (const std::string &name, std::string *ptr, const std::string &doc)
 
void DisableOption (const std::string &name)
 	If called after registering an option and before calling Read(), disables that option from being used. More...
 
template<typename T >
void RegisterStandard (const std::string &name, T *ptr, const std::string &doc)
 	This one is used for registering standard parameters of all the programs. More...
 
int Read (int argc, const char *const *argv)
 //解析命令行选项并且给解析选项登记的变量赋值
 
void PrintUsage (bool print_command_line=false)
 	Prints the usage documentation [provided in the constructor]. More...
 
void PrintConfig (std::ostream &os)
 	Prints the actual configuration of all the registered variables. More...
 
void ReadConfigFile (const std::string &filename)
 	Reads the options values from a config file. More...
 
int NumArgs () const
 	Number of positional parameters (c.f. argc-1). More...
 
std::string GetArg (int param) const
 	Returns one of the positional parameters; 1-based indexing for argc/argv compatibility. More...
 
std::string GetOptArg (int param) const
```

**Read()**:这个函数必须在所有变量登记完毕后进行调用

### SimpleOptions

# **特征提取**

## DeltaFeatures类

![Collaboration graph](http://www.kaldi-asr.org/doc/classkaldi_1_1DeltaFeatures__coll__graph.png)

**公有成员函数**

```c++
DeltaFeatures (const DeltaFeaturesOptions &opts)
void Process (const MatrixBase< BaseFloat > &input_feats, int32 frame, VectorBase< BaseFloat > *output_frame) const
```

**私有属性**

```c++
DeltaFeaturesOptions opts_
std::vector< Vector< BaseFloat >> scales_
```



# TransitionModel类

**公有成员函数**

```c++
 	TransitionModel (const ContextDependencyInterface &ctx_dep, const HmmTopology &hmm_topo)
 	初始化对象 [e.g. More...
 
 	TransitionModel ()
 	不带任何参数的构造函数: typically used prior to calling Read. More...
 
void Read (std::istream &is, bool binary)
 
void Write (std::ostream &os, bool binary) const
 
const HmmTopology & 	GetTopo () const
 	return reference to HMM-topology object. More...
 
bool IsFinal (int32 trans_id) const
 
bool IsSelfLoop (int32 trans_id) const
 
int32 NumTransitionIds () const
 	返回transition-ids的个数 (note, these are one-based). More...
 
int32 NumTransitionIndices (int32 trans_state) const
 	Returns the number of transition-indices for a particular transition-state. More...
 
int32 NumTransitionStates () const
 	Returns the total number of transition-states (note, these are one-based). More...
 
int32 NumPdfs () const
 
int32 NumPhones () const
 
const std::vector< int32 > & 	GetPhones () const
 	Returns a sorted, unique list of phones. More...
 
BaseFloat GetTransitionProb (int32 trans_id) const
 
BaseFloat GetTransitionLogProb (int32 trans_id) const
 
BaseFloat GetTransitionLogProbIgnoringSelfLoops (int32 trans_id) const
 	Returns the log-probability of a particular non-self-loop transition after subtracting the probability mass of the self-loop and renormalizing; will crash if called on a self-loop. More...
 
BaseFloat GetNonSelfLoopLogProb (int32 trans_state) const
 	Returns the log-prob of the non-self-loop probability mass for this transition state. More...
 
void MleUpdate (const Vector< double > &stats, const MleTransitionUpdateConfig &cfg, BaseFloat *objf_impr_out, BaseFloat *count_out)
 	Does Maximum Likelihood estimation. More...
 
void MapUpdate (const Vector< double > &stats, const MapTransitionUpdateConfig &cfg, BaseFloat *objf_impr_out, BaseFloat *count_out)
 	Does Maximum A Posteriori (MAP) estimation. More...
 
void Print (std::ostream &os, const std::vector< std::string > &phone_names, const Vector< double > *occs=NULL)
 	Print will print the transition model in a human-readable way, for purposes of human inspection. More...
 
void InitStats (Vector< double > *stats) const
 
void Accumulate (BaseFloat prob, int32 trans_id, Vector< double > *stats) const
 
bool Compatible (const TransitionModel &other) const
 	returns true if all the integer class members are identical (but does not compare the transition probabilities. More...
```

**整数映射函数**

```c++
int32 TupleToTransitionState (int32 phone, int32 hmm_state, int32 pdf, int32 self_loop_pdf) const
 
int32 PairToTransitionId (int32 trans_state, int32 trans_index) const
 
int32 TransitionIdToTransitionState (int32 trans_id) const
 
int32 TransitionIdToTransitionIndex (int32 trans_id) const
 
int32 TransitionStateToPhone (int32 trans_state) const
 
int32 TransitionStateToHmmState (int32 trans_state) const
 
int32 TransitionStateToForwardPdfClass (int32 trans_state) const
 
int32 TransitionStateToSelfLoopPdfClass (int32 trans_state) const
 
int32 TransitionStateToForwardPdf (int32 trans_state) const
 
int32 TransitionStateToSelfLoopPdf (int32 trans_state) const
 
int32 SelfLoopOf (int32 trans_state) const
 
int32 TransitionIdToPdf (int32 trans_id) const
 
int32 TransitionIdToPdfFast (int32 trans_id) const
 
int32 TransitionIdToPhone (int32 trans_id) const
 
int32 TransitionIdToPdfClass (int32 trans_id) const
 
int32 TransitionIdToHmmState (int32 trans_id) const
```



## **MfccOptions**

包含了对于计算MFCC特征的基本选项

**公有成员函数**

```c++
MfccOptions ()
void Register (OptionsItf *opts)
```

**公有属性**

```c++
FrameExtractionOptions frame_opts
 
MelBanksOptions mel_opts
 
int32 num_ceps
 
bool use_energy
 
BaseFloat energy_floor
 
bool raw_energy
 
BaseFloat cepstral_lifter
 
bool 	htk_compat
```

