#include "domain/command_builder.h"

CommandSpec CommandBuilder::build(const Item &item, const PathResolver &resolver, const QString &homeDirectory)
{
    CommandSpec spec;
    spec.program = QStringLiteral("cmd.exe");
    spec.arguments << (item.keepConsole ? QStringLiteral("/k") : QStringLiteral("/c"));
    spec.arguments << item.command;
    spec.workingDirectory = item.workingDir.trimmed().isEmpty()
        ? homeDirectory
        : resolver.expand(item.workingDir);
    return spec;
}
