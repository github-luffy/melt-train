/**
 *  ==============================================================================
 *
 *          \file   ObjectiveFunction.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-05-10 22:37:44.351396
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef OBJECTIVE_FUNCTION_H_
#define OBJECTIVE_FUNCTION_H_
#include "Dataset.h"
#include "random_util.h"
#include "common_util.h"
namespace gezi {

	class ObjectiveFunction
	{
	public:
		Dataset& Dataset;
	protected:
		Fvec _weights; //@? ��������ܹؼ�
		bool _bestStepRankingRegressionTrees = false;
		Fvec _gradient;
		int _gradSamplingRate;
		Float _learningRate;
		Float _maxTreeOutput = std::numeric_limits<Float>::max();
		static const int _queryThreadChunkSize = 100;
		Random _rnd;

	public:
		ObjectiveFunction(gezi::Dataset& dataset, Float learningRate, Float maxTreeOutput, int gradSamplingRate, bool useBestStepRankingRegressionTree, int randomNumberGeneratorSeed)
			:Dataset(dataset), _rnd(randomNumberGeneratorSeed)
		{
			_learningRate = learningRate;
			_maxTreeOutput = maxTreeOutput;
			_gradSamplingRate = gradSamplingRate;
			_bestStepRankingRegressionTrees = useBestStepRankingRegressionTree;
			_gradient.resize(Dataset.NumDocs);
			_weights.resize(Dataset.NumDocs);
		}

		Fvec& Weights()
		{
			return _weights;
		}

		const Fvec& Weights() const
		{
			return _weights;
		}

		virtual Fvec& GetGradient(const Fvec& scores) = 0;
	

	protected:
		//virtual void GetGradientInOneQuery(int query, const Fvec& scores) = 0;
		//���ȥ���麯��֮�� �ٶ��ܴ�6.32726 s ->6.19447 s �����麯��֮������ �ر���������ڲ�Ƕ���ѭ���ں����麯�� ��������function���
		//���߸ɴ��ô���copy�Ĵ��� ȥ������ڲ��麯��
		//std::function<void(int, const Fvec&)> GetGradientInOneQuery;
	};
	typedef shared_ptr<ObjectiveFunction> ObjectiveFunctionPtr;

	template<typename Derived>
	class ObjectiveFunctionImpl : public ObjectiveFunction
	{
	public:
		using ObjectiveFunction::ObjectiveFunction;
		//@TODOԭ�������100������ ÿ���̴߳���100��query`
		virtual Fvec& GetGradient(const Fvec& scores)
		{
			int sampleIndex = _rnd.Next(_gradSamplingRate);
#pragma omp parallel for
			for (int query = 0; query < Dataset.NumDocs; query++)
			{
				if ((query % _gradSamplingRate) == sampleIndex)
				{
					static_cast<Derived*>(this)->GetGradientInOneQuery(query, scores);
				}
			}
			/*	Pvector(scores)
			Pvector(_gradient)*/
			return _gradient;
		}
	};
	
}  //----end of namespace gezi

#endif  //----end of OBJECTIVE_FUNCTION_H_