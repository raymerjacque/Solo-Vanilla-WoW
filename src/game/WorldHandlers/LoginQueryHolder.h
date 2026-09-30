#ifndef MANGOS_LOGINQUERYHOLDER_H
#define MANGOS_LOGINQUERYHOLDER_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Database/SqlOperations.h"

class LoginQueryHolder : public SqlQueryHolder
{
private:
    uint32 m_accountId;
    ObjectGuid m_guid;
public:
    LoginQueryHolder(uint32 accountId, ObjectGuid guid)
        : m_accountId(accountId), m_guid(guid) { }
    ObjectGuid GetGuid() const { return m_guid; }
    uint32 GetAccountId() const { return m_accountId; }
    bool Initialize();
};

#endif // MANGOS_LOGINQUERYHOLDER_H
