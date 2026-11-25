# Licenciamento e PCC
**Online (PCC, Supabase):** o plugin envia `sha256(chave|BURNIX)`, `machineId` e um nonce para `POST /functions/v1/validate`. A resposta vem assinada (Ed25519); o plugin só confia se a assinatura (chave pública embebida), o nonce, a máquina e o produto baterem certo. Sem rede, usa a última resposta assinada durante 14 dias. A chave em claro nunca sai do computador. Protocolo: `Source/Shared/PccProtocol.h`. Servidor: `pcc/supabase/` (migração SQL + Edge Function).
**Offline (fallback):** PBKDF2-HMAC-SHA256, 60000 iterações. Os hashes **já não estão no repositório**: `Source/Shared/Secrets.h` é gerado por `tools/gen_secrets.py`, ignorado pelo git e injectado no CI pelo segredo `BURNIX_SECRETS_B64`. Sem ele o build compila sem senhas offline (só período aberto e PCC).
**Deploy do PCC (uma vez):**
1. `python3 tools/gen_pcc_keys.py` → guarda `PRIVADO_pcc_chave.txt` (nunca no GitHub). A chave pública (64 hex) vai para `BX_PCC_PUBKEY` no CMakeLists.
2. Supabase → SQL Editor: correr `pcc/supabase/migrations/*.sql`.
3. `supabase secrets set PCC_SIGNING_KEY=<privada base64>` e `supabase functions deploy validate --no-verify-jwt`.
4. Criar licenças: `select * from pcc_new_burnix_license('Nome','email','País','venda', null);` (ou pelo painel PCC, plugin `burnix`).
**Atenção:** os hashes antigos continuam no histórico do git. Considere-os comprometidos: gere novos (`gen_secrets.py`), actualize o segredo `BURNIX_SECRETS_B64` e use PCC. Tornar o repositório privado é decisão do proprietário.

**Ligar o BURNIX ao PCC que já existe (o mesmo projeto Supabase e a mesma chave pública dos outros plugins):**
1. Supabase → SQL Editor: correr `pcc/supabase/migrations/20261119_burnix_license.sql` (cria `pcc_new_burnix_license`).
2. Na Edge Function `validate`, acrescentar `"BURNIX"` à lista `PRODUCTS` (sem apagar os plugins que já lá estão) e voltar a
   publicar: `supabase functions deploy validate --no-verify-jwt`. O ficheiro deste repositório já tem `PRIMEQPRO` e `BURNIX`.
3. Criar licenças: `select * from pcc_new_burnix_license('Nome','email','País','venda', null);` (chaves `BNX-XXXX-XXXX-XXXX`).
4. Senha master offline: `python3 tools/gen_secrets.py --master "<a master de sempre>"` e colocar o conteúdo de `secrets.b64`
   no segredo `BURNIX_SECRETS_B64` do GitHub (Settings → Secrets → Actions). Sem ele o build sai sem senhas offline
   (PCC; o período aberto terminou na 1.0.1).
