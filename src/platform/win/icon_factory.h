#ifndef WINTOOLS_PLATFORM_ICON_FACTORY_H
#define WINTOOLS_PLATFORM_ICON_FACTORY_H

#include "domain/item.h"
#include "domain/path_resolver.h"

#include <QIcon>

namespace IconFactory {

QIcon forItem(const Item &item, const PathResolver &resolver);
QIcon applicationIcon();

} // namespace IconFactory

#endif // WINTOOLS_PLATFORM_ICON_FACTORY_H
