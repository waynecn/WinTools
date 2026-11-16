#ifndef WINTOOLS_PLATFORM_PROCESS_ENUMERATOR_H
#define WINTOOLS_PLATFORM_PROCESS_ENUMERATOR_H

#include "domain/interfaces.h"

class ProcessEnumerator : public IProcessEnumerator
{
public:
    QStringList runningExecutablePaths() const override;
};

#endif // WINTOOLS_PLATFORM_PROCESS_ENUMERATOR_H
