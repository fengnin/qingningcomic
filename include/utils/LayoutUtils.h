#ifndef UTILS_LAYOUTUTILS_H
#define UTILS_LAYOUTUTILS_H

#include <QLayout>
#include <QWidget>
#include <QList>

namespace LayoutUtils {

inline void clearLayout(QLayout* layout, bool deleteWidgets = true)
{
    if (!layout) {
        return;
    }

    QList<QWidget*> widgetsToDelete;
    QLayoutItem* item = nullptr;
    
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->disconnect();
            widget->setParent(nullptr);
            if (deleteWidgets) {
                widgetsToDelete.append(widget);
            }
        } else if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout, deleteWidgets);
        }
        delete item;
    }

    if (deleteWidgets) {
        qDeleteAll(widgetsToDelete);
    }
}

inline void clearContainerLayout(QWidget* container, bool deleteWidgets = true)
{
    if (!container) {
        return;
    }
    clearLayout(container->layout(), deleteWidgets);
}

}

#endif // UTILS_LAYOUTUTILS_H
