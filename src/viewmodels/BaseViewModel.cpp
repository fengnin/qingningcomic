#include "viewmodels/BaseViewModel.h"

BaseViewModel::BaseViewModel(QObject* parent)
    : QObject(parent)
{
}

BaseViewModel::~BaseViewModel()
{
}

void BaseViewModel::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged(busy);
    }
}

void BaseViewModel::setLastError(const QString& error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

void BaseViewModel::clearError()
{
    m_lastError.clear();
}

void BaseViewModel::withBusy(const std::function<void()>& operation)
{
    if (!operation) { return; }
    
    setBusy(true);
    clearError();
    operation();
    setBusy(false);
}
