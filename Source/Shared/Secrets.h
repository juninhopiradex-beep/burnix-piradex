// GERADO por tools/gen_secrets.py. So salt + hash PBKDF2 (nunca a senha em claro).
#pragma once
#include "Licensing.h"

namespace pq { namespace secrets
{
inline constexpr pq::lic::PwEntry kPasswords[] =
{
    { "master", pq::lic::Role::Master, "fac6515489d4f2e4cd991c5f324c97ba", "0ff3eca99f7f35f65134ef6d96d5c5c5901353d1d287131955dc906a7444db1f", 0, 0, 0 },
    { "beta-01", pq::lic::Role::Beta,   "ee7cda3e546088030f4eb4d165cf1ca8", "4804afe1fc84163bdc299287c15cbd49d53c4875395221e5be45e02c42146e72", 2027, 6, 30 },
};
inline constexpr int kPasswordCount = (int) (sizeof (kPasswords) / sizeof (kPasswords[0]));
} } // namespace pq::secrets
