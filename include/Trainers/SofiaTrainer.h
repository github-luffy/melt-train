/**
 *  ==============================================================================
 *
 *          \file   Trainers/SofiaTrainer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-11-20 05:15:35.156835
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TRAINERS__SOFIA_TRAINER_H_
#define TRAINERS__SOFIA_TRAINER_H_
#include "common_util.h"

#include "ThirdTrainer.h"
#include "Predictors/SofiaPredictor.h" //not used, will generate LinearPredictor
#include "string2argcargv.h"

class SfWeightVector;
class SfDataSet;
namespace gezi {

	class SofiaTrainer : public ThirdTrainer
	{
	public:
		SofiaTrainer()
		{
			_classiferSettings = "--lambda 0.001 --iterations 50000";
		}

		virtual string GetPredictorName() override
		{
			return "sofia";
		}

	protected:
		virtual void ShowHelp() override;

		SfDataSet Instances2SfDataSet(Instances& instances);
		virtual void Initialize(Instances& instances) override;
		virtual void InnerTrain(Instances& instances) override;
		virtual void Finalize(Instances& instances) override;

		/// <summary>
		/// Return the raw margin from the decision hyperplane
		/// </summary>		
		Float Margin(const Vector& features)
		{
			return _bias + _weights.dot(features);
		}

	private:
		SfWeightVector* w = NULL;
	};

}  //----end of namespace gezi

#endif  //----end of TRAINERS__SOFIA_TRAINER_H_
