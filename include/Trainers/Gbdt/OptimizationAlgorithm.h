/**
 *  ==============================================================================
 *
 *          \file   OptimizationAlgorithm.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-05-09 14:35:32.002966
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef OPTIMIZATION_ALGORITHM_H_
#define OPTIMIZATION_ALGORITHM_H_
#include "common_def.h"
#include "TreeLearner.h"
#include "RegressionTree.h"
#include "ObjectiveFunction.h"
#include "ScoreTracker.h"
#include "Ensemble.h"
#include "IStepSearch.h"
#include "RecursiveRegressionTree.h"
#include "Prediction/Instances/Instances.h"
namespace gezi {

	class OptimizationAlgorithm
	{
	public:
		OptimizationAlgorithm(gezi::Ensemble& ensemble, const Dataset& trainData, Fvec& initTrainScores)
			:Ensemble(ensemble)
		{

		}

		OptimizationAlgorithm(gezi::Ensemble& ensemble)
			:Ensemble(ensemble)
		{

		}

		//因为c++的构造函数中不能有虚函数 ConstructScoreTracker 单独提出 和c#不一样  @TODO check好像gcc支持构造函数有虚函数类似c#？
		virtual void Initialize(const Dataset& trainData, Fvec& initTrainScores)
		{
			TrainingScores = ConstructScoreTracker("itrain", trainData, initTrainScores);
			TrackedScores.push_back(TrainingScores);
		}

		virtual RegressionTree& TrainingIteration(const BitArray& activeFeatures) = 0;
		virtual ScoreTrackerPtr ConstructScoreTracker(string name, const Dataset& set, Fvec& initScores)
		{
			return make_shared<ScoreTracker>(name, set, initScores);
		}
		virtual ScoreTrackerPtr ConstructScoreTracker(string name, const Instances& instances, Fvec& initScores)
		{
			return make_shared<ScoreTracker>(name, instances, initScores);
		}

		virtual void FinalizeLearning(int bestIteration)
		{
			if (bestIteration != Ensemble.NumTrees())
			{
				Ensemble.RemoveAfter(std::max(bestIteration, 0));
				TrackedScores.clear();
			}
		}

		//当前设计有一点乱 如果bagging nbag > 1那么 会重新对每个test set再生成tracker 但是由于引用的score不变 所以整体效果一样 继续累积score
		//或者外围手动move一下 TrackedScores,不过性能意义几乎没有
		ScoreTrackerPtr GetScoreTracker(string name, const Instances& set, Fvec& InitScores)
		{
			for (ScoreTrackerPtr st : TrackedScores)
			{
				if (st->DatasetName == name)
				{
					return st;
				}
			}
			ScoreTrackerPtr newTracker = ConstructScoreTracker(name, set, InitScores);
			TrackedScores.push_back(newTracker);
			return newTracker;
		}

		void SetTrainingData(const Dataset& trainData, Fvec& initTrainScores)
		{
			TrainingScores = ConstructScoreTracker("itrain", trainData, initTrainScores);
			TrackedScores[0] = TrainingScores;
		}

		virtual void SmoothTree(RegressionTree& tree, Float smoothing)
		{
			AutoTimer timer("SmoothTree");
			if (smoothing != 0.0)
			{ //@FIMXE check smoothTree 在多机环境是否符合预期？ 目前没有考虑多机影响 是否Partitioning需要特殊处理? TLC里面有
				shared_ptr<RecursiveRegressionTree> regularizer = make_shared<RecursiveRegressionTree>(tree, TreeLearner->Partitioning, 0);
				double rootNodeOutput = regularizer->GetWeightedOutput();
				//@TODO may be can speed up withoud resing RecursiveRegressionTree
				regularizer->SmoothLeafOutputs(rootNodeOutput, smoothing);
			}
		}

		virtual void UpdateAllScores(RegressionTree& tree)
		{
			//dynamic_pointer_cast<IStepSearch>(ObjectiveFunction))->AdjustTreeOutputsAutoTimer timer("UpdateAllScores");
			for (ScoreTrackerPtr t : TrackedScores)
			{
				UpdateScores(t, tree);
			}
		}

		virtual void UpdateScores(ScoreTrackerPtr t, RegressionTree& tree)
		{
			if (t == TrainingScores)
			{
				if (AdjustTreeOutputsOverride != nullptr)
				{ //@TODO
					//VLOG(2) << "AdjustTreeOutputsOverride != nullptr";
				}
				else
				{
					//VLOG(2) << "t->AddScores(tree, TreeLearner->Partitioning, 1.0)";
					t->AddScores(tree, TreeLearner->Partitioning, 1.0);
				}
			}
			else
			{
				//VLOG(2) << "t->AddScores(tree,1.0)";
				t->AddScores(tree, 1.0);
			}
		}

	public:
		Ensemble& Ensemble;
		IStepSearchPtr AdjustTreeOutputsOverride = nullptr;
		TreeLearnerPtr TreeLearner = nullptr;
		ObjectiveFunctionPtr ObjectiveFunction = nullptr;
		Float Smoothing = 0.0;
		vector<ScoreTrackerPtr> TrackedScores;
		ScoreTrackerPtr TrainingScores = nullptr;
	protected:
		bool _useFastTrainingScoresUpdate = true;
	};

	typedef shared_ptr<OptimizationAlgorithm> OptimizationAlgorithmPtr;
}  //----end of namespace gezi

#endif  //----end of OPTIMIZATION_ALGORITHM_H_
