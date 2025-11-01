
from django.db import models

class LuminosidadeReading(models.Model):
    """Armazena o histórico de leituras do sensor Wemos."""
    valor = models.IntegerField()
    modo = models.CharField(max_length=50)
    timestamp = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"[{self.timestamp}] Valor: {self.valor}"

class ControlState(models.Model):
    """
    Armazena o ÚLTIMO comando recebido do app.
    Esta tabela deve ter apenas UMA linha (singleton, id=1).
    """
    modo = models.CharField(max_length=10, default='auto')
    r = models.IntegerField(default=0)
    g = models.IntegerField(default=0)
    b = models.IntegerField(default=0)

    def __str__(self):
        return f"Modo: {self.modo} (R:{self.r}, G:{self.g}, B:{self.b})"