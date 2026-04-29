#ifndef CHANGEREQUESTEXECUTIONFLOWUTILS_H
#define CHANGEREQUESTEXECUTIONFLOWUTILS_H

#include <QtGlobal>

namespace ChangeRequestExecutionFlowUtils {

inline int computeOperationProgress(int currentIndex, int totalOperations, int baseProgress = 40, int progressSpan = 45)
{
    if (totalOperations <= 0) {
        return qBound(0, baseProgress + progressSpan, 100);
    }

    const int completed = qBound(0, currentIndex + 1, totalOperations);
    const int progress = baseProgress + (completed * progressSpan / totalOperations);
    return qBound(0, progress, 100);
}

}

#endif // CHANGEREQUESTEXECUTIONFLOWUTILS_H
