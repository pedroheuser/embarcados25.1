# Troubleshooting: Servidor Recusando Conexões HTTP

## Checklist de Diagnóstico

Execute os seguintes comandos no servidor VPS para identificar o problema:

### 1. Verificar se o Nginx está rodando

```bash
sudo systemctl status nginx
```

**Se não estiver rodando:**
```bash
sudo systemctl start nginx
sudo systemctl enable nginx
```

**Se houver erros, verificar configuração:**
```bash
sudo nginx -t
```

### 2. Verificar se o Gunicorn/Django está rodando

```bash
sudo systemctl status iot-backend
```

**Se não estiver rodando:**
```bash
sudo systemctl start iot-backend
sudo systemctl enable iot-backend
```

**Verificar logs para erros:**
```bash
sudo journalctl -u iot-backend -n 50 --no-pager
```

### 3. Verificar se as portas estão escutando

```bash
# Verificar se Nginx está na porta 80
sudo netstat -tlnp | grep :80

# Verificar se Gunicorn está na porta 8000
sudo netstat -tlnp | grep :8000

# Ou usar ss (alternativa mais moderna)
sudo ss -tlnp | grep :80
sudo ss -tlnp | grep :8000
```

### 4. Verificar Firewall

```bash
# Ver status do UFW
sudo ufw status

# Se ativo, verificar se portas estão abertas
sudo ufw status numbered

# Se necessário, abrir portas
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw reload
```

### 5. Testar Gunicorn localmente

```bash
# Conectar ao servidor e testar diretamente
curl http://127.0.0.1:8000/api/status/

# Se funcionar localmente, o problema é no Nginx
# Se não funcionar, o problema é no Gunicorn/Django
```

### 6. Testar Nginx

```bash
# Testar se Nginx responde
curl http://localhost/api/status/
# ou
curl http://seu-dominio.ufsc.br/api/status/
```

### 7. Verificar logs do Nginx

```bash
# Logs de erro
sudo tail -f /var/log/nginx/error.log

# Logs de acesso
sudo tail -f /var/log/nginx/access.log

# Verificar logs em tempo real enquanto tenta acessar
```

### 8. Verificar configuração do Nginx

Verifique se o arquivo `/etc/nginx/sites-available/iot-backend` está correto:

```nginx
server {
    listen 80;
    server_name seu-dominio.ufsc.br IP_DO_VPS;

    # ... resto da configuração ...
    
    location / {
        proxy_pass http://127.0.0.1:8000;  # ← Verificar se está correto
        # ... resto ...
    }
}
```

**Testar configuração:**
```bash
sudo nginx -t
```

**Se houver erros, corrigir e recarregar:**
```bash
sudo nginx -t  # Verificar novamente
sudo systemctl reload nginx
```

### 9. Verificar permissões e caminhos

```bash
# Verificar se os caminhos no systemd service estão corretos
cat /etc/systemd/system/iot-backend.service

# Verificar se o usuário tem permissões
ls -la /home/seu_usuario/embarcados/backend/

# Verificar se o ambiente virtual existe
ls -la /home/seu_usuario/embarcados/backend/venv/bin/gunicorn
```

### 10. Problemas Comuns e Soluções

#### Problema A: "Connection refused" no curl

**Sintomas:** `curl: (7) Failed to connect to ... Connection refused`

**Possíveis causas:**
- Gunicorn não está rodando
- Porta 8000 não está aberta
- Firewall bloqueando

**Solução:**
```bash
# Verificar Gunicorn
sudo systemctl restart iot-backend
sudo systemctl status iot-backend

# Verificar se está escutando
sudo ss -tlnp | grep 8000
```

#### Problema B: "502 Bad Gateway"

**Sintomas:** Nginx retorna 502

**Possíveis causas:**
- Gunicorn não está rodando ou caiu
- Nginx não consegue se conectar ao Gunicorn
- Erro na aplicação Django

**Solução:**
```bash
# Verificar Gunicorn
sudo systemctl status iot-backend
sudo journalctl -u iot-backend -n 50

# Reiniciar Gunicorn
sudo systemctl restart iot-backend

# Verificar se Django tem erros
cd ~/embarcados/backend
source venv/bin/activate
python manage.py check
```

#### Problema C: "403 Forbidden"

**Sintomas:** Nginx retorna 403

**Possíveis causas:**
- Problema de permissões
- Configuração incorreta do Nginx

**Solução:**
```bash
# Verificar permissões
sudo chown -R seu_usuario:www-data ~/embarcados/backend
sudo chmod -R 755 ~/embarcados/backend
```

#### Problema D: Porta 80 já está em uso

**Sintomas:** `nginx: [emerg] bind() to 0.0.0.0:80 failed (98: Address already in use)`

**Solução:**
```bash
# Verificar o que está usando a porta 80
sudo lsof -i :80
# ou
sudo netstat -tlnp | grep :80

# Se for outro serviço, parar ou reconfigurar
```

### 11. Script de Diagnóstico Completo

Execute este script para uma verificação completa:

```bash
#!/bin/bash
echo "=== Verificando Nginx ==="
sudo systemctl status nginx --no-pager | head -5

echo -e "\n=== Verificando Gunicorn ==="
sudo systemctl status iot-backend --no-pager | head -5

echo -e "\n=== Verificando Portas ==="
echo "Porta 80 (Nginx):"
sudo ss -tlnp | grep :80 || echo "Porta 80 não está escutando!"
echo "Porta 8000 (Gunicorn):"
sudo ss -tlnp | grep :8000 || echo "Porta 8000 não está escutando!"

echo -e "\n=== Testando Gunicorn localmente ==="
curl -s http://127.0.0.1:8000/api/status/ || echo "Gunicorn não responde!"

echo -e "\n=== Testando Nginx ==="
curl -s http://localhost/api/status/ || echo "Nginx não responde!"

echo -e "\n=== Últimos erros do Gunicorn ==="
sudo journalctl -u iot-backend -n 10 --no-pager

echo -e "\n=== Últimos erros do Nginx ==="
sudo tail -n 5 /var/log/nginx/error.log
```

Salve como `diagnostico.sh`, dê permissão de execução e execute:
```bash
chmod +x diagnostico.sh
./diagnostico.sh
```

### 12. Reiniciar Tudo

Se nada funcionar, tente reiniciar todos os serviços:

```bash
# Parar serviços
sudo systemctl stop nginx
sudo systemctl stop iot-backend

# Aguardar alguns segundos
sleep 3

# Iniciar serviços
sudo systemctl start iot-backend
sleep 2
sudo systemctl start nginx

# Verificar status
sudo systemctl status iot-backend
sudo systemctl status nginx
```

## Próximos Passos

Após identificar o problema, me envie:
1. O resultado do comando `sudo systemctl status nginx`
2. O resultado do comando `sudo systemctl status iot-backend`
3. Os últimos logs: `sudo journalctl -u iot-backend -n 30`
4. O erro específico que aparece ao tentar conectar

Com essas informações, posso ajudar a resolver o problema específico!

