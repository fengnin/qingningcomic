#ifndef BASEVIEWMODEL_H
#define BASEVIEWMODEL_H

#include <QObject>
#include <QSharedPointer>
#include <functional>
#include "services/ServiceContainer.h"

class BaseViewModel : public QObject
{
    Q_OBJECT

public:
    explicit BaseViewModel(QObject* parent = nullptr);
    virtual ~BaseViewModel();
    
    virtual void initialize() {}
    virtual void cleanup() {}
    
    bool isBusy() const { return m_busy; }
    QString lastError() const { return m_lastError; }

signals:
    void busyChanged(bool busy);
    void errorOccurred(const QString& error);
    void operationCompleted(const QString& operation);

protected:
    void setBusy(bool busy);
    void setLastError(const QString& error);
    void clearError();
    
    template<typename ServiceType, typename ServiceSingleton>
    ServiceType* getService(ServiceType* (ServiceContainer::*getter)() const,
                            ServiceType* (*singletonGetter)())
    {
        ServiceType* service = (ServiceContainer::instance()->*getter)();
        if (!service) {
            service = singletonGetter();
        }
        return service;
    }
    
    void withBusy(const std::function<void()>& operation);

private:
    bool m_busy = false;
    QString m_lastError;
};

#endif // BASEVIEWMODEL_H
