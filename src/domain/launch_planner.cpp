#include "domain/launch_planner.h"

#include "domain/command_builder.h"

#include <QFileInfo>

QString launchStatusLabel(LaunchStatus status)
{
    switch (status) {
    case LaunchStatus::Success:
        return QStringLiteral("成功");
    case LaunchStatus::Skipped:
        return QStringLiteral("已跳过");
    case LaunchStatus::Failed:
        return QStringLiteral("失败");
    case LaunchStatus::Invalid:
        return QStringLiteral("无效");
    }
    return QString();
}

LaunchPlanner::LaunchPlanner(const PathResolver *resolver, const IProcessEnumerator *enumerator,
                             QString homeDirectory)
    : m_resolver(resolver)
    , m_enumerator(enumerator)
    , m_homeDirectory(std::move(homeDirectory))
{
}

LaunchPlan LaunchPlanner::plan(const Group &group, bool forceRelaunch) const
{
    LaunchPlan plan;
    const QSet<QString> running = collectRunningPaths();
    for (const Item &item : group.items) {
        plan.items.append(planItem(item, forceRelaunch, running));
    }
    return plan;
}

LaunchPlan LaunchPlanner::plan(const Item &item, bool forceRelaunch) const
{
    LaunchPlan plan;
    plan.items.append(planItem(item, forceRelaunch, collectRunningPaths()));
    return plan;
}

QSet<QString> LaunchPlanner::collectRunningPaths() const
{
    QSet<QString> paths;
    if (!m_enumerator) {
        return paths;
    }
    const QStringList executables = m_enumerator->runningExecutablePaths();
    for (const QString &executable : executables) {
        paths.insert(m_resolver->normalized(executable));
    }
    return paths;
}

PlannedItem LaunchPlanner::planItem(const Item &item, bool forceRelaunch,
                                    const QSet<QString> &runningPaths) const
{
    PlannedItem planned;
    planned.item = item;
    planned.action.target = item.targetText();

    const PathStatus pathStatus = m_resolver->classify(item);
    if (pathStatus != PathStatus::Valid) {
        planned.status = LaunchStatus::Invalid;
        planned.message = PathResolver::pathStatusText(pathStatus);
        return planned;
    }

    switch (item.type) {
    case ItemType::Directory:
    case ItemType::File: {
        planned.action.kind = LaunchAction::Kind::ShellOpen;
        planned.action.target = m_resolver->expand(item.path);
        planned.message = QStringLiteral("已发起打开");
        break;
    }
    case ItemType::Application: {
        const QString executable = m_resolver->expand(item.path);
        if (!forceRelaunch && runningPaths.contains(m_resolver->normalized(executable))) {
            planned.status = LaunchStatus::Skipped;
            planned.message = QStringLiteral("应用已在运行，已跳过");
            break;
        }
        planned.action.kind = LaunchAction::Kind::StartProcess;
        planned.action.program = executable;
        planned.action.workingDirectory = QFileInfo(executable).absolutePath();
        planned.action.target = executable;
        planned.message = QStringLiteral("已发起启动");
        break;
    }
    case ItemType::Url: {
        if (item.browser.mode == BrowserMode::SystemDefault) {
            planned.action.kind = LaunchAction::Kind::ShellOpen;
            planned.action.target = item.url;
            planned.message = QStringLiteral("已使用系统默认浏览器打开");
            break;
        }
        const QString browser = m_resolver->expand(item.browser.programPath);
        if (browser.trimmed().isEmpty()) {
            planned.status = LaunchStatus::Invalid;
            planned.message = QStringLiteral("浏览器路径为空");
            break;
        }
        // 浏览器必须是存在的文件。
        Item browserItem = Item::make(ItemType::Application);
        browserItem.path = item.browser.programPath;
        const PathStatus browserStatus = m_resolver->classify(browserItem);
        if (browserStatus != PathStatus::Valid) {
            planned.status = LaunchStatus::Invalid;
            planned.message = QStringLiteral("浏览器路径无效");
            break;
        }
        planned.action.kind = LaunchAction::Kind::StartProcess;
        planned.action.program = browser;
        planned.action.arguments << item.url;
        planned.action.target = browser + QStringLiteral(" ") + item.url;
        planned.message = QStringLiteral("已用指定浏览器打开");
        break;
    }
    case ItemType::Command: {
        const CommandSpec spec = CommandBuilder::build(item, *m_resolver, m_homeDirectory);
        planned.action.kind = LaunchAction::Kind::StartProcess;
        planned.action.program = spec.program;
        planned.action.arguments = spec.arguments;
        planned.action.workingDirectory = spec.workingDirectory;
        planned.action.target = item.command;
        planned.message = QStringLiteral("已发起命令");
        break;
    }
    }

    return planned;
}
