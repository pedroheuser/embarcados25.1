from rest_framework import serializers

from .models import LuminosidadeReading


class LuminosidadeSerializer(serializers.ModelSerializer):
    class Meta:
        model = LuminosidadeReading
        fields = ['id', 'valor', 'modo', 'timestamp']


LuminosidadeReadingSerializer = LuminosidadeSerializer

