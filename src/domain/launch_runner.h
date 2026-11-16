#ifndef WINTOOLS_DOMAIN_LAUNCH_RUNNER_H
#define WINTOOLS_DOMAIN_LAUNCH_RUNNER_H

#include "domain/interfaces.h"
#include "domain/launch_planner.h"
#include "domain/logger.h"

#include <QString>
#include <QVector>

struct LaunchResult
{
    QString name;
    ItemType type = ItemType::Directory;
    LaunchStatus status = LaunchStatus::Success;
    QString target;
    QString message;
};

class LaunchRunner
{
public:
    LaunchRunner(ILauncher *launcher, Logger *logger);

    QVector<LaunchResult> run(const LaunchPlan &plan, const QString &actionName);

private:
    bool execute(const LaunchAction &action, QString *error);

    ILauncher *m_launcher;
    Logger *m_logger;
};

#endif // WINTOOLS_DOMAIN_LAUNCH_RUNNER_H
