// GERADO por tools/gen_secrets.py. So salt + hash PBKDF2 (nunca a senha em claro).
#pragma once
#include "Licensing.h"

namespace pq { namespace secrets
{
inline constexpr pq::lic::PwEntry kPasswords[] =
{
    { "master", pq::lic::Role::Master,
      "fb24f5a3de8b24476a74e85ef76b1df5",
      "7751f8cd7bd4ed067a76156d152c7fc7d72a402c592ab028f441123e240fa508", 0, 0, 0 },

    { "beta-01", pq::lic::Role::Beta,
      "f294d6b3944d5123cd638c417a726b16",
      "04f297559b2999b73395a9b51fe595ad818ea75d67db6503db551ed4a7edafd1", 2026, 12, 31 },
};
inline constexpr int kPasswordCount = (int) (sizeof (kPasswords) / sizeof (kPasswords[0]));
} } // namespace pq::secrets
