-- =============================================================================
--  PCC — licenças do BURNIX by PIRADEX
--  Pré-requisito: a migração de validação (pcc_find_license) já instalada pelo PrimEQ Pro (20261103_primeqpro_validate.sql).
--  Colar em Supabase -> SQL Editor -> Run. Seguro repetir.
--  Exemplo:  select * from public.pcc_new_burnix_license('João Silva','joao@x.com','Angola','venda', null);
-- =============================================================================
create or replace function public.pcc_new_burnix_license(p_cliente text, p_email text, p_pais text, p_tipo text, p_validade date)
returns table(chave text, validade date)
language plpgsql
security definer
set search_path = public, extensions
as $$
declare k text;
begin
  k := 'BNX-' || upper(substr(encode(gen_random_bytes(2),'hex'),1,4)) || '-' || upper(substr(encode(gen_random_bytes(2),'hex'),1,4)) || '-' || upper(substr(encode(gen_random_bytes(2),'hex'),1,4));
  insert into public.licencas(cliente,email,pais,plugin,chave,tipo,validade,estado)
  values (p_cliente,p_email,p_pais,'burnix',k,coalesce(p_tipo,'venda'),p_validade,'ativo');
  return query select k, p_validade;
end $$;
revoke all on function public.pcc_new_burnix_license(text,text,text,text,date) from public, anon;
grant execute on function public.pcc_new_burnix_license(text,text,text,text,date) to authenticated, service_role;
