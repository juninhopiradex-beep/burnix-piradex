// GERADO por tools/gen_secrets.py. So salt + hash PBKDF2 (nunca a senha em claro).
#pragma once
#include "Licensing.h"

namespace pq { namespace secrets
{
inline constexpr pq::lic::PwEntry kPasswords[] =
{
    { "master", pq::lic::Role::Master, "422d21371ff1d675647455e25578d4c1", "8908b0b33109fcf24a2f0a62bccd82c4c0e8f9d46267da1d8cc6edaedba3682f", 0, 0, 0 },
    { "beta-01", pq::lic::Role::Beta,   "ee7cda3e546088030f4eb4d165cf1ca8", "4804afe1fc84163bdc299287c15cbd49d53c4875395221e5be45e02c42146e72", 2027, 6, 30 },
};
inline constexpr int kPasswordCount = (int) (sizeof (kPasswords) / sizeof (kPasswords[0]));
} } // namespace pq::secrets
