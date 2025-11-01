
from rest_framework import serializers
from .models import LuminosidadeReading, ControlState

class LuminosidadeReadingSerializer(serializers.ModelSerializer):
    """ Serializer para o histórico de leituras """
    class Meta:
        model = LuminosidadeReading
        fields = ['id', 'valor', 'modo', 'timestamp']


# --- Serializers para o ControlState ---

class ColorSerializer(serializers.Serializer):
    """ Serializer auxiliar para o objeto 'cor' aninhado """
    r = serializers.IntegerField(min_value=0, max_value=255)
    g = serializers.IntegerField(min_value=0, max_value=255)
    b = serializers.IntegerField(min_value=0, max_value=255)

class ControlStateSerializer(serializers.Serializer):
    """ Serializer principal que o App e o Wemos usam """
    modo = serializers.ChoiceField(choices=['auto', 'manual'])
    cor = ColorSerializer(required=False, allow_null=True)