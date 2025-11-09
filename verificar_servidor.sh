#!/bin/bash
# Script rápido para verificar status do servidor

echo "=========================================="
echo "DIAGNÓSTICO RÁPIDO DO SERVIDOR"
echo "=========================================="

echo -e "\n[1] Verificando Nginx..."
if systemctl is-active --quiet nginx; then
    echo "✓ Nginx está rodando"
else
    echo "✗ Nginx NÃO está rodando"
    echo "  Execute: sudo systemctl start nginx"
fi

echo -e "\n[2] Verificando Gunicorn (iot-backend)..."
if systemctl is-active --quiet iot-backend; then
    echo "✓ iot-backend está rodando"
else
    echo "✗ iot-backend NÃO está rodando"
    echo "  Execute: sudo systemctl start iot-backend"
fi

echo -e "\n[3] Verificando portas..."
if ss -tlnp | grep -q ":80 "; then
    echo "✓ Porta 80 (HTTP) está escutando"
else
    echo "✗ Porta 80 NÃO está escutando"
fi

if ss -tlnp | grep -q ":8000 "; then
    echo "✓ Porta 8000 (Gunicorn) está escutando"
else
    echo "✗ Porta 8000 NÃO está escutando"
fi

echo -e "\n[4] Testando Gunicorn localmente..."
if curl -s http://127.0.0.1:8000/api/status/ > /dev/null; then
    echo "✓ Gunicorn responde corretamente"
    curl -s http://127.0.0.1:8000/api/status/
    echo ""
else
    echo "✗ Gunicorn NÃO responde"
fi

echo -e "\n[5] Testando Nginx..."
if curl -s http://localhost/api/status/ > /dev/null; then
    echo "✓ Nginx responde corretamente"
    curl -s http://localhost/api/status/
    echo ""
else
    echo "✗ Nginx NÃO responde ou erro no proxy"
fi

echo -e "\n[6] Verificando firewall..."
if command -v ufw &> /dev/null; then
    ufw_status=$(sudo ufw status | head -1)
    echo "Status UFW: $ufw_status"
    if echo "$ufw_status" | grep -q "Status: active"; then
        echo "  Portas abertas:"
        sudo ufw status numbered | grep -E "80|443" || echo "  ⚠ Portas 80/443 podem estar fechadas"
    fi
else
    echo "UFW não encontrado (pode usar iptables)"
fi

echo -e "\n[7] Últimos erros do Gunicorn (últimas 5 linhas):"
sudo journalctl -u iot-backend -n 5 --no-pager 2>/dev/null || echo "  Nenhum log encontrado"

echo -e "\n[8] Últimos erros do Nginx (últimas 5 linhas):"
sudo tail -n 5 /var/log/nginx/error.log 2>/dev/null || echo "  Arquivo de log não encontrado"

echo -e "\n=========================================="
echo "Diagnóstico concluído!"
echo "=========================================="

