#ifndef WINTOOLS_DOMAIN_COMMAND_BUILDER_H
#define WINTOOLS_DOMAIN_COMMAND_BUILDER_H

#include "domain/item.h"
#include "domain/path_resolver.h"

#include <QString>
#include <QStringList>

struct CommandSpec
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
};

class CommandBuilder
{
public:
    static CommandSpec build(const Item &item, const PathResolver &resolver, const QString &homeDirectory);
};

#endif // WINTOOLS_DOMAIN_COMMAND_BUILDER_H
