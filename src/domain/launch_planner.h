#ifndef WINTOOLS_DOMAIN_LAUNCH_PLANNER_H
#define WINTOOLS_DOMAIN_LAUNCH_PLANNER_H

#include "domain/config.h"
#include "domain/interfaces.h"
#include "domain/path_resolver.h"

#include <QSet>
#include <QString>
#include <QVector>

enum class LaunchStatus
{
    Success,
    Skipped,
    Failed,
    Invalid
};

QString launchStatusLabel(LaunchStatus status);

struct LaunchAction
{
    enum class Kind
    {
        None,
        ShellOpen,
        StartProcess
    };

    Kind kind = Kind::None;
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QString target;
};

struct PlannedItem
{
    Item item;
    LaunchStatus status = LaunchStatus::Success;
    QString message;
    LaunchAction action;
};

struct LaunchPlan
{
    QVector<PlannedItem> items;
};

class LaunchPlanner
{
public:
    LaunchPlanner(const PathResolver *resolver, const IProcessEnumerator *enumerator,
                  QString homeDirectory);

    LaunchPlan plan(const Group &group, bool forceRelaunch) const;
    LaunchPlan plan(const Item &item, bool forceRelaunch) const;

private:
    PlannedItem planItem(const Item &item, bool forceRelaunch, const QSet<QString> &runningPaths) const;
    QSet<QString> collectRunningPaths() const;

    const PathResolver *m_resolver;
    const IProcessEnumerator *m_enumerator;
    QString m_homeDirectory;
};

#endif // WINTOOLS_DOMAIN_LAUNCH_PLANNER_H
