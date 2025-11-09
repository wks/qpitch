#pragma once

#include <QMutex>
#include <cstdint>
#include <vector>

// ** TEMPORARY BUFFERS USED FOR VISUALIZATION ** //
class VisualizationData {
public:
    VisualizationData(unsigned int plotData_size);

    QMutex mutex;                               //!< Ensure the QPitchCore thread doesn't write to the buffers when the UI thread is drawing
	std::vector<double>	plotSample;	            //!< Buffer used to store time samples used for visualization
	std::vector<double>	plotAutoCorr;	        //!< Buffer used to store autocorrelation samples used for visualization
    double              estimatedFrequency;     //!< Estimated frequency
	unsigned int		plotData_size;	        //!< Total number of samples used for visualization
};
