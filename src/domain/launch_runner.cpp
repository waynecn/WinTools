#include "domain/launch_runner.h"

LaunchRunner::LaunchRunner(ILauncher *launcher, Logger *logger)
    : m_launcher(launcher)
    , m_logger(logger)
{
}

QVector<LaunchResult> LaunchRunner::run(const LaunchPlan &plan, const QString &actionName)
{
    QVector<LaunchResult> results;
    int success = 0;
    int skipped = 0;
    int failed = 0;
    int invalid = 0;

    for (const PlannedItem &planned : plan.items) {
        LaunchResult result;
        result.name = planned.item.name;
        result.type = planned.item.type;
        result.status = planned.status;
        result.target = planned.action.kind == LaunchAction::Kind::None
            ? planned.item.targetText()
            : planned.action.target;
        result.message = planned.message;

        if (planned.status == LaunchStatus::Success) {
            QString error;
            if (!execute(planned.action, &error)) {
                result.status = LaunchStatus::Failed;
                result.message = error;
            }
        }

        switch (result.status) {
        case LaunchStatus::Success:
            ++success;
            break;
        case LaunchStatus::Skipped:
            ++skipped;
            break;
        case LaunchStatus::Failed:
            ++failed;
            break;
        case LaunchStatus::Invalid:
            ++invalid;
            break;
        }

        if (m_logger) {
            m_logger->log(QStringLiteral("%1 | %2 | %3 | %4 | %5 | %6")
                              .arg(actionName,
                                   result.name,
                                   itemTypeLabel(result.type),
                                   result.target,
                                   launchStatusLabel(result.status),
                                   result.message));
        }
        results.append(result);
    }

    if (m_logger) {
        m_logger->log(QStringLiteral("%1 | 汇总 | 成功 %2，跳过 %3，失败 %4，无效 %5")
                          .arg(actionName)
                          .arg(success)
                          .arg(skipped)
                          .arg(failed)
                          .arg(invalid));
    }
    return results;
}

bool LaunchRunner::execute(const LaunchAction &action, QString *error)
{
    if (!m_launcher) {
        if (error) {
            *error = QStringLiteral("启动器未初始化");
        }
        return false;
    }
    if (action.kind == LaunchAction::Kind::ShellOpen) {
        return m_launcher->shellOpen(action.target, error);
    }
    if (action.kind == LaunchAction::Kind::StartProcess) {
        return m_launcher->startDetached(action.program, action.arguments,
                                         action.workingDirectory, error);
    }
    if (error) {
        *error = QStringLiteral("没有可执行的启动动作");
    }
    return false;
}
