/**
 *  ==============================================================================
 *
 *          \file   Trainers/LibSVMTrainer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-11-20 05:15:42.677732
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TRAINERS__LIB_S_V_M_TRAINER_H_
#define TRAINERS__LIB_S_V_M_TRAINER_H_

#include "ThirdTrainer.h"

namespace gezi {

class LibSVMTrainer : public ThirdTrainer
{
public:

	virtual string GetPredictorName() override
	{
		return "LibSVM";
	}

protected:
private:

};

}  //----end of namespace gezi

#endif  //----end of TRAINERS__LIB_S_V_M_TRAINER_H_
