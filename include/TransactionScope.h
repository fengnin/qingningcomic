#ifndef TRANSACTIONSCOPE_H
#define TRANSACTIONSCOPE_H

#include "DatabaseManager.h"

class TransactionScope
{
public:
    explicit TransactionScope(DatabaseManager* db)
        : m_db(db)
        , m_committed(false)
        , m_active(false)
    {
        if (m_db) {
            m_active = m_db->beginTransaction();
        }
    }
    
    ~TransactionScope()
    {
        if (m_active && !m_committed) {
            m_db->rollback();
        }
    }
    
    bool isActive() const { return m_active; }
    
    bool commit()
    {
        if (!m_active) {
            return true;
        }
        
        if (m_db->commit()) {
            m_committed = true;
            return true;
        }
        return false;
    }
    
    void rollback()
    {
        if (m_active && !m_committed) {
            m_db->rollback();
            m_committed = true;
        }
    }
    
private:
    DatabaseManager* m_db;
    bool m_committed;
    bool m_active;
};

#endif // TRANSACTIONSCOPE_H
