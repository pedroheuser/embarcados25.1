from django.db import models


class LuminosidadeReading(models.Model):
    valor = models.IntegerField()
    modo = models.CharField(max_length=50)
    timestamp = models.DateTimeField(auto_now_add=True)

    def __str__(self) -> str:
        return f"Luminosidade {self.valor} ({self.modo})"

