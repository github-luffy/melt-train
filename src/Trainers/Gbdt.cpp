/**
 *  ==============================================================================
 *
 *          \file   Gbdt.cpp
 *
 *        \author   chenghuige
 *
 *          \date   2014-05-12 10:09:15.775908
 *
 *  \Description:
 *  ==============================================================================
 */

#include "common_util.h"
#include "Trainers/Gbdt/Gbdt.h"
#include "Trainers/Gbdt/BinaryClassificationGbdt.h"
#include "Trainers/Gbdt/RegressionGbdt.h"
#include "Trainers/Gbdt/RankingGbdt.h"

DECLARE_bool(calibrate);
DECLARE_string(calibrator);
DECLARE_uint64(numCali);

DECLARE_uint64(rs);
DECLARE_int32(iter);
DEFINE_int32(ntree, 100, "numTrees: Number of trees/iteraiton number");
DECLARE_double(lr);
DEFINE_double(lambda, 0.2, "same as lr, but notice hack for lr which will fail to set 0.001");
DEFINE_int32(nl, 20, "numLeaves: Number of leaves maximam allowed in each regression tree");
DEFINE_int32(mil, 10, "minInstancesInLeaf: Minimal instances in leaves allowd");
//@TODO 需要测试一下mdepth 在并行的时候是否ok
DEFINE_int32(mdepth, -1, "maxDepth: max depth allowed for the each tree, -1 means no depth restriction");
DEFINE_bool(bsr, false, "bestStepRankingRegressionTrees: @TODO");
DEFINE_double(sp, 0.1, "Sparsity level needed to use sparse feature representation, if 0.3 means be sparsify only if real data less then 30%, 0-1 the smaller more dense and faster but use more memeory");
DEFINE_double(ff, 1, "The fraction of features (chosen randomly) to use on each iteration");
DEFINE_double(sf, 1, "The fraction of features (chosen randomly) to use on each split");
DEFINE_int32(mb, 255, "Maximum number of distinct values (bins) per feature");
DEFINE_int32(ps, -1, "The number of histograms in the pool (between 2 and numLeaves - 1)");
DEFINE_bool(psc, false, "Wether first randomly select a subset of features and then pick the feature that maximizes gain or post do this: @FIXME");

DEFINE_int32(bag, 0, "Number of trees in each bag (0 for disabling bagging)");
DEFINE_double(bagfrac, 0.7, "Percentage of training queries used in each bag");
//bagging 应该还是有问题。。。 关键是TrainSet的问题？ NumDocs 等等 scores等等
DEFINE_int32(nbag, 1, "NumBags|if nbag > 1 then we actually has nbag * numtress = totalTrees"); 
DEFINE_double(nbagfrac, 0.7, "Percentage of training queries used in each bag");
DECLARE_bool(bstrap);

DEFINE_double(bsfrac, 1.0, "BootStrapFraction|traditional bootstrap sampling will use all data");

DEFINE_double(entropy, 0, "entropyCoefficient|sets the entropy coefficient, which encourages the algorithm to prefer balanced splits in the tree (splits where an equal number of training documents go in each direction)");

//@TODO 并行没有test smooth是否ok
DEFINE_double(smooth, 0, "Smoothing| paramter for tree regularization, can take values from zero to one, zero means that no smoothing takes place while one means that trees are smoothed to the extent that no learning takes place at all. Smoothing essentially mixes each the output value of each leaf in the tree with the output values of its neighboring leaves. Leaves with many training documents will be modified less than leaves with few training documents. Smoothing trees is a form of regularization, which promoted generalization but may cause harm if used too aggressively");

DEFINE_bool(rstart, false, "randomStart|Initialize with one random tree");

DEFINE_int32(maxfs, 0, "maxFeaturesShow| max print feature num");

DEFINE_int32(distributeMode, 0, "0:feature spliting, each use full feature space mem but calculating only on sub feature 1:feature spliting, each use only sub feature space  2:instance spliting");

//-------------for gbdt ranking|lambdamart
DEFINE_bool(fzl, false, "filterZeroLambdas|");

namespace gezi {

	void Gbdt::ParseArgs()
	{
		_args = CreateArguments();

		_args->calibrateOutput = FLAGS_calibrate; //@TODO to Trainer deal
		_args->calibratorName = FLAGS_calibrator;

		if (!are_same(FLAGS_lr, 0.001)) //@TODO Float 判断相同 判断是否是0 另外如果用户再输入0.001不起作用了就 不符合逻辑了
			_args->learningRate = FLAGS_lr;
		else
			_args->learningRate = FLAGS_lambda;

		if (FLAGS_iter != 50000) //复用LinearSVM部分定义的参数iter,其默认值是50000
			_args->numTrees = FLAGS_iter;
		else
			_args->numTrees = FLAGS_ntree;

		_args->maxDepth = FLAGS_mdepth;

		if (FLAGS_nl < 2)
			LOG(WARNING) << "The number of leaves must be >= 2, so use default value instead";
		else
			_args->numLeaves = FLAGS_nl;

		_args->randSeed = FLAGS_rs;

		_args->minInstancesInLeaf = FLAGS_mil;

		_args->bestStepRankingRegressionTrees = FLAGS_bsr;
		_args->sparsifyRatio = FLAGS_sp;

		_args->featureFraction = FLAGS_ff;
		_args->splitFraction = FLAGS_sf;
		_args->preSplitCheck = FLAGS_psc;

		_args->baggingSize = FLAGS_bag;
		_args->baggingTrainFraction = FLAGS_bagfrac;
		_args->numBags = FLAGS_nbag;
		_args->nbaggingTrainFraction = FLAGS_nbagfrac;

		_args->boostStrap = FLAGS_bstrap;
		_args->bootStrapFraction = FLAGS_bsfrac;

		if (_args->baggingSize != 0)
		{
			CHECK_EQ(_args->numTrees % _args->baggingSize, 0) << "numTrees must be n * baggingSize";
		}

		_args->entropyCoefficient = FLAGS_entropy;
		_args->smoothing = FLAGS_smooth;

		_args->maxBins = FLAGS_mb;

		//---- doing more 这里感觉就是对于historygram做了pool存储？ 为什么容量 *2/3？容量大一点不好吗
		//如果是numLeaves - 1那么所有的非root分裂 都是有parent存储的 可以substract 主要是内存考虑吧？
		_args->histogramPoolSize = FLAGS_ps;
		if (_args->histogramPoolSize < 2)
		{
			_args->histogramPoolSize = (_args->numLeaves * 2) / 3;
		}
		if (_args->histogramPoolSize > (_args->numLeaves - 1))
		{
			_args->histogramPoolSize = _args->numLeaves - 1;
		}

		_args->randomStart = FLAGS_rstart;

		_args->maxFeaturesShow = FLAGS_maxfs;

		_args->distributeMode = static_cast<DistributeMode>(FLAGS_distributeMode);
	}

	void BinaryClassificationGbdt::ParseClassificationArgs()
	{
		_args->maxCalibrationExamples = FLAGS_numCali;
	}

	void RegressionGbdt::ParseRegressionArgs()
	{

	}

	void RankingGbdt::ParseRankingArgs()
	{
		_args->filterZeroLambdas = FLAGS_fzl;
	}
}
